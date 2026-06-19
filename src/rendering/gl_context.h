#pragma once

#include <string>

struct GLFWwindow;

class GLContext
{
public:
    GLContext(int width, int height, const std::string& title);
    ~GLContext();

    GLFWwindow* window() const { return m_window; }
    bool shouldClose() const;
    void beginFrame();
    void endFrame();

    int width() const { return m_width; }
    int height() const { return m_height; }
    float dpiScale() const { return m_dpiScale; }

    GLContext(const GLContext&) = delete;
    GLContext& operator=(const GLContext&) = delete;

private:
    void initGLFW();
    void initOpenGL();
    void initImGuiBackends();
    void calculateDpiScale();

    GLFWwindow* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    float m_dpiScale = 1.0f;
    std::string m_title;
};
