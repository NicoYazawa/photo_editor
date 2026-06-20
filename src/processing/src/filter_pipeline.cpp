#include "filter_pipeline.h"
#include "filter_base.h"

#include "fbo.h"
#include "fullscreen_quad.h"
#include "shader.h"

#include <glad/glad.h>
#include <cstdio>
#include <unordered_map>

// Shared vertex shader for all filter passes
static const char* kCommonVert = R"(
#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

// Global shader cache — compile each filter's shader once
static std::unordered_map<std::string, std::unique_ptr<Shader>> s_shaderCache;

static Shader* getOrCompileShader(const std::string& key,
                                   const char* vertSrc,
                                   const char* fragSrc)
{
    auto it = s_shaderCache.find(key);
    if (it != s_shaderCache.end() && it->second && it->second->valid())
        return it->second.get();

    auto shader = std::make_unique<Shader>(vertSrc, fragSrc);
    if (!shader->valid())
    {
        fprintf(stderr, "[Pipeline] Shader compile failed for '%s'\n", key.c_str());
        return nullptr;
    }
    Shader* ptr = shader.get();
    s_shaderCache[key] = std::move(shader);
    return ptr;
}

FilterPipeline::FilterPipeline()
    : m_quad(std::make_unique<FullscreenQuad>())
{
}

FilterPipeline::~FilterPipeline() = default;

void FilterPipeline::ensureFBOs(int width, int height)
{
    if (m_fboWidth == width && m_fboHeight == height &&
        m_fboA && m_fboA->valid() && m_fboB && m_fboB->valid())
        return;

    m_fboA = std::make_unique<FBO>();
    m_fboB = std::make_unique<FBO>();

    if (!m_fboA->create(width, height) || !m_fboB->create(width, height))
    {
        fprintf(stderr, "[Pipeline] Failed to create FBOs\n");
        m_fboA.reset();
        m_fboB.reset();
    }

    m_fboWidth = width;
    m_fboHeight = height;
}

GLuint FilterPipeline::apply(GLuint inputTexture,
                              int width, int height,
                              const std::vector<FilterBase*>& filters)
{
    if (filters.empty())
    {
        m_currentResult = inputTexture;
        return m_currentResult;
    }

    ensureFBOs(width, height);
    if (!m_fboA || !m_fboB)
        return inputTexture;

    GLuint srcTex = inputTexture;
    int ping = 0; // 0=write to A, 1=write to B

    for (auto* filter : filters)
    {
        Shader* shader = getOrCompileShader(
            filter->key(), kCommonVert, filter->fragmentShaderSource());
        if (!shader)
        {
            fprintf(stderr, "[Pipeline] Skipping filter '%s' (shader failed)\n",
                    filter->name().c_str());
            continue;
        }

        int passes = filter->passCount();
        for (int p = 0; p < passes; ++p)
        {
            FBO& target = (ping == 0) ? *m_fboA : *m_fboB;

            target.bind();

            shader->use();
            shader->setInt("uTexture", 0);
            shader->setVec2("uTexelSize", 1.0f / width, 1.0f / height);
            filter->setUniformsForPass(*shader, p);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, srcTex);

            m_quad->draw();

            target.unbind();
            srcTex = target.colorTexture();
            ping = 1 - ping;
        }
    }

    m_currentResult = srcTex;
    return m_currentResult;
}

GLuint FilterPipeline::singlePass(GLuint srcTexture, FilterBase* filter,
                                   int width, int height, int pass)
{
    (void)width;
    (void)height;

    m_fboA->bind();

    Shader shader(kCommonVert, filter->fragmentShaderSource());
    if (!shader.valid())
    {
        m_fboA->unbind();
        return srcTexture;
    }

    shader.use();
    shader.setInt("uTexture", 0);
    filter->setUniformsForPass(shader, pass);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTexture);

    m_quad->draw();

    m_fboA->unbind();
    return m_fboA->colorTexture();
}
