#include "ui_manager.h"
#include "image_panel.h"

#include "filter_base.h"
#include "filter_registry.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdio>

// Forward declare tinyfiledialogs functions if we use them,
// otherwise use a simple platform file dialog approach
// For now, we'll use a minimal implementation

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <comdef.h>  // for _bstr_t
#elif defined(__linux__)
// Use GTK or zenity
#include <cstdlib>
#endif

UIManager::UIManager()
    : m_originalPanel(std::make_unique<ImagePanel>("Original"))
    , m_resultPanel(std::make_unique<ImagePanel>("Result"))
{
    m_lastFpsTime = ImGui::GetTime();
}

UIManager::~UIManager() = default;

void UIManager::render()
{
    setupDockSpace();
    renderMenuBar();

    // Keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    bool ctrl = io.KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z))
    {
        if (m_onUndo) m_onUndo();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y))
    {
        if (m_onRedo) m_onRedo();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O))
    {
        std::string path = openFileDialog();
        if (!path.empty() && m_onOpenFile)
            m_onOpenFile(path);
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S))
    {
        std::string path = saveFileDialog();
        if (!path.empty() && m_onSaveFile)
            m_onSaveFile(path);
    }

    // Image panels — initial size 512x512
    ImGui::SetNextWindowSize(ImVec2(512, 512), ImGuiCond_FirstUseEver);
    m_originalPanel->render(m_originalTexId, m_originalW, m_originalH);

    ImGui::SetNextWindowSize(ImVec2(512, 512), ImGuiCond_FirstUseEver);
    m_resultPanel->render(m_resultTexId, m_resultW, m_resultH);

    // Filter config panel
    ImGui::Begin("Filter Config");
    if (m_configFilter && m_configFilter->hasConfigUI())
    {
        ImGui::Text("Filter: %s", m_currentFilterName.c_str());
        ImGui::Separator();
        m_configFilter->renderConfigUI();
    }
    else if (!m_currentFilterName.empty())
    {
        ImGui::Text("Filter: %s", m_currentFilterName.c_str());
        ImGui::TextDisabled("(no parameters to configure)");
    }
    else
    {
        ImGui::TextDisabled("Select a filter from the Filter menu to view its parameters");
    }
    ImGui::End();

    renderStatusBar();

    // FPS counter
    m_frameCount++;
    double now = ImGui::GetTime();
    if (now - m_lastFpsTime >= 1.0)
    {
        m_fps = static_cast<float>(m_frameCount / (now - m_lastFpsTime));
        m_frameCount = 0;
        m_lastFpsTime = now;
    }
}

void UIManager::setupDockSpace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockFlags = ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus |
                                  ImGuiWindowFlags_NoNavFocus |
                                  ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("MainDockSpace", nullptr, dockFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockId = ImGui::GetID("MainDock");

    // DockBuilder must run BEFORE DockSpace to set up initial layout
    if (!ImGui::DockBuilderGetNode(dockId))
    {
        ImGui::DockBuilderRemoveNode(dockId);
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockId, ImGui::GetMainViewport()->Size);

        // Layout: menu bar (top) | Filter Config (left) | Original | Result
        ImGuiID dockMenu  = 0, dockBody = 0;
        ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Up, 0.05f, &dockMenu, &dockBody);

        ImGuiID dockFilter = 0, dockCenter = 0;
        ImGui::DockBuilderSplitNode(dockBody, ImGuiDir_Left, 0.18f, &dockFilter, &dockCenter);

        ImGuiID dockOrig = 0, dockResult = 0;
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Left, 0.5f, &dockOrig, &dockResult);

        ImGui::DockBuilderDockWindow("Filter Config", dockFilter);
        ImGui::DockBuilderDockWindow("Original", dockOrig);
        ImGui::DockBuilderDockWindow("Result", dockResult);

        ImGui::DockBuilderFinish(dockId);
    }

    ImGui::DockSpace(dockId);
    ImGui::End();
}

void UIManager::renderMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        // File menu
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
            {
                std::string path = openFileDialog();
                if (!path.empty() && m_onOpenFile)
                    m_onOpenFile(path);
            }

            if (ImGui::MenuItem("Save As...", "Ctrl+S"))
            {
                std::string path = saveFileDialog();
                if (!path.empty() && m_onSaveFile)
                    m_onSaveFile(path);
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                // Will be handled by the app
            }
            ImGui::EndMenu();
        }

        // Edit menu
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_onUndo != nullptr))
            {
                if (m_onUndo) m_onUndo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_onRedo != nullptr))
            {
                if (m_onRedo) m_onRedo();
            }
            ImGui::EndMenu();
        }

        // Filter menu — dynamically populated from registry
        if (ImGui::BeginMenu("Filter"))
        {
            auto& registry = FilterRegistry::instance();
            for (const auto& category : registry.categories())
            {
                if (ImGui::BeginMenu(category.c_str()))
                {
                    for (const auto& key : registry.keysInCategory(category))
                    {
                        std::string displayName = registry.name(key);
                        if (ImGui::MenuItem(displayName.c_str()))
                        {
                            if (m_onApplyFilter)
                                m_onApplyFilter(key);
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }

        // Help menu
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("About", nullptr, false, false);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void UIManager::renderStatusBar()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 statusPos = ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 22);
    ImVec2 statusSize = ImVec2(viewport->Size.x, 22);

    ImGui::SetNextWindowPos(statusPos);
    ImGui::SetNextWindowSize(statusSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                              ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoResize |
                              ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("StatusBar", nullptr, flags);
    ImGui::PopStyleVar(3);

    char buf[256];
    snprintf(buf, sizeof(buf), "%s | Filter: %s | %.1f FPS",
             m_statusInfo.c_str(),
             m_currentFilterName.empty() ? "None" : m_currentFilterName.c_str(),
             static_cast<double>(m_fps));
    ImGui::TextUnformatted(buf);

    ImGui::End();
}

void UIManager::showFilterConfig(FilterBase* filter)
{
    m_configFilter = filter;
}

void UIManager::setOriginalTexture(GLuint id, int w, int h)
{
    m_originalTexId = id;
    m_originalW = w;
    m_originalH = h;
    m_originalPanel->resetView();
}

void UIManager::setResultTexture(GLuint id, int w, int h)
{
    m_resultTexId = id;
    m_resultW = w;
    m_resultH = h;
    m_resultPanel->resetView();
}

void UIManager::setStatusInfo(const std::string& info)
{
    m_statusInfo = info;
}

void UIManager::setCurrentFilterName(const std::string& name)
{
    m_currentFilterName = name;
}

#ifdef _WIN32
// Windows file dialog using COM
static std::string wcharToUtf8(const wchar_t* wstr)
{
    if (!wstr) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr, nullptr);
    return result;
}

std::string UIManager::openFileDialog()
{
    std::string result;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInit = SUCCEEDED(hr);

    IFileOpenDialog* pDialog = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pDialog));
    if (SUCCEEDED(hr))
    {
        // Set file type filters
        COMDLG_FILTERSPEC filters[] = {
            { L"Images", L"*.png;*.jpg;*.jpeg;*.bmp;*.tga" },
            { L"All Files", L"*.*" },
        };
        pDialog->SetFileTypes(2, filters);

        hr = pDialog->Show(nullptr);
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem = nullptr;
            hr = pDialog->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszPath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                if (SUCCEEDED(hr))
                {
                    result = wcharToUtf8(pszPath);
                    CoTaskMemFree(pszPath);
                }
                pItem->Release();
            }
        }
        pDialog->Release();
    }

    if (comInit) CoUninitialize();
    return result;
}

std::string UIManager::saveFileDialog()
{
    std::string result;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool comInit = SUCCEEDED(hr);

    IFileSaveDialog* pDialog = nullptr;
    hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
                          IID_IFileSaveDialog, reinterpret_cast<void**>(&pDialog));
    if (SUCCEEDED(hr))
    {
        COMDLG_FILTERSPEC filters[] = {
            { L"PNG Image", L"*.png" },
            { L"JPEG Image", L"*.jpg;*.jpeg" },
        };
        pDialog->SetFileTypes(2, filters);
        pDialog->SetDefaultExtension(L"png");

        hr = pDialog->Show(nullptr);
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem = nullptr;
            hr = pDialog->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszPath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                if (SUCCEEDED(hr))
                {
                    result = wcharToUtf8(pszPath);
                    CoTaskMemFree(pszPath);
                }
                pItem->Release();
            }
        }
        pDialog->Release();
    }

    if (comInit) CoUninitialize();
    return result;
}

#else
// Linux/macOS fallback - use zenity or a simple path input
std::string UIManager::openFileDialog()
{
    // Simple fallback: return empty, user would implement with zenity/GTK
    return {};
}

std::string UIManager::saveFileDialog()
{
    return {};
}
#endif
