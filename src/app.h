#pragma once

#include <glad/glad.h>

#include <memory>
#include <string>
#include <vector>

class GLContext;
class Texture;
class UIManager;
class FilterBase;

struct HistoryEntry;

class App
{
public:
    App();
    ~App();

    void run();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

private:
    void loadImage(const std::string& path);
    void saveImage(const std::string& path);
    void applyFilter(const std::string& key);
    void executePipeline();

    void undo();
    void redo();
    void pushHistory(const std::string& filterName);
    void clearRedoStack();

    // Read back pixels from a GL texture via temporary FBO
    std::vector<unsigned char> readTexturePixels(GLuint texId,
                                                  int width, int height);

    std::unique_ptr<GLContext> m_glContext;
    std::unique_ptr<UIManager> m_uiManager;

    // Textures
    std::unique_ptr<Texture> m_originalTexture;
    std::unique_ptr<Texture> m_resultTexture;

    // Image info
    std::string m_filePath;
    int m_imageWidth = 0;
    int m_imageHeight = 0;

    // Filter chain
    std::vector<std::unique_ptr<FilterBase>> m_filters;

    // History
    std::vector<HistoryEntry> m_history;
    int m_historyIndex = -1;

    // Redo stack: filters that were popped by undo
    std::vector<std::unique_ptr<FilterBase>> m_redoStack;

    bool m_running = true;
};
