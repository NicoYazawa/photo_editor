#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <glad/glad.h>

class FilterBase;
class FilterPipeline;
class ImagePanel;

class UIManager
{
public:
    UIManager();
    ~UIManager();

    void render();

    // Callbacks for file operations
    using FileCallback = std::function<void(const std::string& path)>;
    void setOnOpenFile(FileCallback cb) { m_onOpenFile = std::move(cb); }
    void setOnSaveFile(FileCallback cb) { m_onSaveFile = std::move(cb); }

    // Callback for filter application
    using FilterCallback = std::function<void(const std::string& filterKey)>;
    void setOnApplyFilter(FilterCallback cb) { m_onApplyFilter = std::move(cb); }

    // Callbacks for edit operations
    using SimpleCallback = std::function<void()>;
    void setOnUndo(SimpleCallback cb) { m_onUndo = std::move(cb); }
    void setOnRedo(SimpleCallback cb) { m_onRedo = std::move(cb); }

    // State setters for display
    void setOriginalTexture(GLuint id, int w, int h);
    void setResultTexture(GLuint id, int w, int h);
    void setStatusInfo(const std::string& info);
    void setCurrentFilterName(const std::string& name);

    // Filter config panel — set which filter's params to show
    // Pass nullptr to show placeholder
    void showFilterConfig(FilterBase* filter);

    // File dialog helpers
    std::string openFileDialog();
    std::string saveFileDialog();

private:
    void renderMenuBar();
    void renderStatusBar();
    void setupDockSpace();

    std::unique_ptr<ImagePanel> m_originalPanel;
    std::unique_ptr<ImagePanel> m_resultPanel;

    // Texture info for display
    GLuint m_originalTexId = 0;
    int m_originalW = 0, m_originalH = 0;
    GLuint m_resultTexId = 0;
    int m_resultW = 0, m_resultH = 0;

    // Status
    std::string m_statusInfo;
    std::string m_currentFilterName;
    float m_fps = 0.0f;

    // Callbacks
    FileCallback m_onOpenFile;
    FileCallback m_onSaveFile;
    FilterCallback m_onApplyFilter;
    SimpleCallback m_onUndo;
    SimpleCallback m_onRedo;

    // Active filter for config panel
    FilterBase* m_configFilter = nullptr;

    // FPS tracking
    double m_lastFpsTime = 0.0;
    int m_frameCount = 0;
};
