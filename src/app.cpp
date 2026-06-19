#include "app.h"

#include "rendering/gl_context.h"
#include "rendering/texture.h"
#include "rendering/fbo.h"
#include "rendering/fullscreen_quad.h"
#include "rendering/shader.h"

#include "processing/filter_base.h"
#include "processing/filter_pipeline.h"
#include "processing/filter_registry.h"

#include "processing/filters/color_filters.h"
#include "processing/filters/blur_filters.h"
#include "processing/filters/edge_filters.h"
#include "processing/filters/threshold_filter.h"

#include "ui/ui_manager.h"
#include "ui/image_panel.h"

#include "utils/image_io.h"

#include <glad/glad.h>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

// ============================================================
// History entry for undo/redo
// ============================================================
struct HistoryEntry
{
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    std::string filterName;
};

// Passthrough shader sources used for texture readback/copy
static const char* kPassthroughVert = R"(
#version 460 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main(){gl_Position=vec4(aPos,0,1);vTexCoord=aTexCoord;}
)";

static const char* kPassthroughFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
void main(){FragColor=texture(uTexture,vTexCoord);}
)";

// ============================================================
// App Implementation
// ============================================================

App::App()
{
    // Create GL context and window
    m_glContext = std::make_unique<GLContext>(1600, 900, "Photo Editor");

    // Register all filters
    auto& reg = FilterRegistry::instance();
    reg.registerFilter("color.grayscale", []{ return std::make_unique<GrayscaleFilter>(); });
    reg.registerFilter("color.invert", []{ return std::make_unique<InvertFilter>(); });
    reg.registerFilter("color.brightness_contrast", []{ return std::make_unique<BrightnessContrastFilter>(); });
    reg.registerFilter("color.color_balance", []{ return std::make_unique<ColorBalanceFilter>(); });
    reg.registerFilter("color.threshold", []{ return std::make_unique<ThresholdFilter>(); });
    reg.registerFilter("blur.gaussian", []{ return std::make_unique<GaussianBlurFilter>(); });
    reg.registerFilter("blur.box", []{ return std::make_unique<BoxBlurFilter>(); });
    reg.registerFilter("edge.sobel", []{ return std::make_unique<SobelFilter>(); });
    reg.registerFilter("edge.laplacian", []{ return std::make_unique<LaplacianFilter>(); });
    reg.registerFilter("enhance.sharpen", []{ return std::make_unique<SharpenFilter>(); });

    // Create UI manager
    m_uiManager = std::make_unique<UIManager>();

    // Wire up callbacks
    m_uiManager->setOnOpenFile([this](const std::string& path) { loadImage(path); });
    m_uiManager->setOnSaveFile([this](const std::string& path) { saveImage(path); });
    m_uiManager->setOnApplyFilter([this](const std::string& key) { applyFilter(key); });
    m_uiManager->setOnUndo([this]() { undo(); });
    m_uiManager->setOnRedo([this]() { redo(); });
}

App::~App() = default;

void App::run()
{
    while (m_running && !m_glContext->shouldClose())
    {
        m_glContext->beginFrame();
        m_uiManager->render();
        m_glContext->endFrame();
    }
}

// ============================================================
// Texture readback helper (FBO + glReadPixels, core-profile safe)
// ============================================================

std::vector<unsigned char> App::readTexturePixels(GLuint texId,
                                                   int width, int height)
{
    std::vector<unsigned char> pixels(width * height * 4);

    FBO readFbo;
    if (!readFbo.create(width, height))
    {
        fprintf(stderr, "[App] readTexturePixels: FBO create failed\n");
        return pixels;
    }

    readFbo.bind();

    Shader shader(kPassthroughVert, kPassthroughFrag);
    FullscreenQuad quad;

    shader.use();
    shader.setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    quad.draw();

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    readFbo.unbind();

    return pixels;
}

// ============================================================
// Image loading
// ============================================================

void App::loadImage(const std::string& path)
{
    ImageData image = ImageIO::load(path);
    if (!image.valid()) return;

    m_filePath = path;
    m_imageWidth = image.width;
    m_imageHeight = image.height;

    // Create or update original texture
    if (!m_originalTexture)
        m_originalTexture = std::make_unique<Texture>();
    m_originalTexture->create(image.width, image.height, image.pixels.data());

    // Initialize result to same as original
    if (!m_resultTexture)
        m_resultTexture = std::make_unique<Texture>();
    m_resultTexture->create(image.width, image.height, image.pixels.data());

    // Clear state
    m_filters.clear();
    m_redoStack.clear();
    m_history.clear();
    m_historyIndex = -1;

    // Update UI
    m_uiManager->setOriginalTexture(m_originalTexture->id(), image.width, image.height);
    m_uiManager->setResultTexture(m_resultTexture->id(), image.width, image.height);

    char buf[256];
    snprintf(buf, sizeof(buf), "%s | %dx%d",
             path.substr(path.find_last_of("/\\") + 1).c_str(),
             image.width, image.height);
    m_uiManager->setStatusInfo(buf);
    m_uiManager->setCurrentFilterName("");

    printf("[App] Image loaded: %s\n", path.c_str());
}

// ============================================================
// Image saving
// ============================================================

void App::saveImage(const std::string& path)
{
    if (!m_resultTexture || !m_resultTexture->valid())
    {
        fprintf(stderr, "[App] No image to save\n");
        return;
    }

    int w = m_resultTexture->width();
    int h = m_resultTexture->height();

    // Read back via FBO (core-profile safe)
    auto pixels = readTexturePixels(m_resultTexture->id(), w, h);

    // stb_image_write expects top-to-bottom; OpenGL reads bottom-to-top
    std::vector<unsigned char> flipped(w * h * 4);
    for (int y = 0; y < h; ++y)
        memcpy(flipped.data() + y * w * 4, pixels.data() + (h - 1 - y) * w * 4, w * 4);

    if (ImageIO::save(path, flipped.data(), w, h))
        printf("[App] Image saved: %s\n", path.c_str());
    else
        fprintf(stderr, "[App] Failed to save: %s\n", path.c_str());
}

// ============================================================
// Filter application
// ============================================================

void App::applyFilter(const std::string& key)
{
    if (!m_resultTexture || !m_resultTexture->valid())
    {
        fprintf(stderr, "[App] No image loaded\n");
        return;
    }

    auto filter = FilterRegistry::instance().create(key);
    if (!filter)
    {
        fprintf(stderr, "[App] Unknown filter: %s\n", key.c_str());
        return;
    }

    // Save history snapshot before applying (with old filter name)
    std::string oldName = m_filters.empty() ? "Original" : m_filters.back()->name();
    pushHistory(oldName);

    // Clear redo stack
    clearRedoStack();

    // Replace entire filter chain with the new single filter
    // (always apply to original, not stacked)
    m_filters.clear();
    m_filters.push_back(std::move(filter));

    // Execute pipeline with the single filter
    executePipeline();

    printf("[App] Applied filter: %s\n", key.c_str());
}

void App::executePipeline()
{
    if (m_filters.empty() || !m_originalTexture)
        return;

    FilterPipeline pipeline;

    std::vector<FilterBase*> filterPtrs;
    for (auto& f : m_filters)
        filterPtrs.push_back(f.get());

    GLuint resultId = pipeline.apply(
        m_originalTexture->id(),
        m_imageWidth, m_imageHeight,
        filterPtrs);

    // Copy pipeline result to m_resultTexture using GPU-side copy (no CPU roundtrip)
    if (resultId && resultId != m_originalTexture->id())
    {
        if (!m_resultTexture)
            m_resultTexture = std::make_unique<Texture>();
        if (!m_resultTexture->valid() ||
            m_resultTexture->width() != m_imageWidth ||
            m_resultTexture->height() != m_imageHeight)
        {
            m_resultTexture->create(m_imageWidth, m_imageHeight, nullptr);
        }

        // GPU-to-GPU copy (GL 4.3+, core profile safe)
        glCopyImageSubData(
            resultId, GL_TEXTURE_2D, 0, 0, 0, 0,
            m_resultTexture->id(), GL_TEXTURE_2D, 0, 0, 0, 0,
            m_imageWidth, m_imageHeight, 1);
    }
    else if (resultId == m_originalTexture->id())
    {
        // No filters: copy original to result via GPU
        if (!m_resultTexture)
            m_resultTexture = std::make_unique<Texture>();
        if (!m_resultTexture->valid() ||
            m_resultTexture->width() != m_imageWidth ||
            m_resultTexture->height() != m_imageHeight)
        {
            m_resultTexture->create(m_imageWidth, m_imageHeight, nullptr);
        }

        glCopyImageSubData(
            m_originalTexture->id(), GL_TEXTURE_2D, 0, 0, 0, 0,
            m_resultTexture->id(), GL_TEXTURE_2D, 0, 0, 0, 0,
            m_imageWidth, m_imageHeight, 1);
    }

    // Update UI
    m_uiManager->setResultTexture(m_resultTexture->id(), m_imageWidth, m_imageHeight);

    if (!m_filters.empty())
    {
        m_uiManager->setCurrentFilterName(m_filters.back()->name());

        // Show filter config for the last filter in chain
        FilterBase* lastFilter = m_filters.back().get();
        m_uiManager->showFilterConfig(lastFilter);
    }
}

// ============================================================
// Undo / Redo
// ============================================================

void App::pushHistory(const std::string& filterName)
{
    if (!m_resultTexture || !m_resultTexture->valid())
        return;

    // Remove future history if branching
    while (static_cast<int>(m_history.size()) > m_historyIndex + 1)
        m_history.pop_back();

    // Read back current result via FBO (core-profile safe)
    HistoryEntry entry;
    entry.width = m_imageWidth;
    entry.height = m_imageHeight;
    entry.filterName = filterName;
    entry.pixels = readTexturePixels(m_resultTexture->id(), m_imageWidth, m_imageHeight);

    m_history.push_back(std::move(entry));
    m_historyIndex = static_cast<int>(m_history.size()) - 1;

    // Limit history size (check image size for memory bound)
    const int kMaxHistory = 20;
    if (static_cast<int>(m_history.size()) > kMaxHistory)
    {
        m_history.erase(m_history.begin());
        m_historyIndex--;
    }
}

void App::undo()
{
    if (m_historyIndex < 0)
        return;

    // Move the last filter to redo stack
    if (!m_filters.empty())
    {
        auto undoneFilter = std::move(m_filters.back());
        m_filters.pop_back();
        m_redoStack.push_back(std::move(undoneFilter));
    }

    // Restore previous snapshot
    const auto& entry = m_history[m_historyIndex];
    if (!m_resultTexture)
        m_resultTexture = std::make_unique<Texture>();
    m_resultTexture->create(entry.width, entry.height, entry.pixels.data());

    m_historyIndex--;

    m_uiManager->setResultTexture(m_resultTexture->id(), m_imageWidth, m_imageHeight);

    if (!m_filters.empty())
    {
        m_uiManager->setCurrentFilterName(m_filters.back()->name());
        m_uiManager->showFilterConfig(m_filters.back().get());
    }
    else
    {
        m_uiManager->setCurrentFilterName("");
        m_uiManager->showFilterConfig(nullptr);
    }

    printf("[App] Undo (filters: %zu, history: %d/%zu, redo: %zu)\n",
           m_filters.size(), m_historyIndex, m_history.size(), m_redoStack.size());
}

void App::redo()
{
    if (m_redoStack.empty())
        return;

    // Restore the filter from redo stack
    auto restoredFilter = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    m_filters.push_back(std::move(restoredFilter));

    // Re-execute the full pipeline to produce the correct result
    executePipeline();

    printf("[App] Redo (filters: %zu, redo: %zu)\n",
           m_filters.size(), m_redoStack.size());
}

void App::clearRedoStack()
{
    m_redoStack.clear();
}
