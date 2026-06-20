#include "gl_context.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>
#include <cstdlib>

static void glfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "[GLFW Error %d] %s\n", error, description);
}

GLContext::GLContext(int width, int height, const std::string& title)
    : m_width(width), m_height(height), m_title(title)
{
    initGLFW();
    initOpenGL();
    initImGuiBackends();
    calculateDpiScale();
}

GLContext::~GLContext()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void GLContext::initGLFW()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync
}

void GLContext::initOpenGL()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to initialize GLAD\n");
        exit(EXIT_FAILURE);
    }

    printf("OpenGL Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version:  %s\n", glGetString(GL_VERSION));
    printf("GLSL Version:    %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

void GLContext::initImGuiBackends()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr; // We'll manage layout ourselves or use LoadIniSettingsFromMemory

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
}

void GLContext::calculateDpiScale()
{
    // Get framebuffer size vs window size to compute DPI scale
    int fbWidth = 0, fbHeight = 0;
    int winWidth = 0, winHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    glfwGetWindowSize(m_window, &winWidth, &winHeight);

    if (winWidth > 0)
        m_dpiScale = static_cast<float>(fbWidth) / static_cast<float>(winWidth);

    // Update stored size to framebuffer dimensions
    m_width = fbWidth;
    m_height = fbHeight;
}

bool GLContext::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void GLContext::beginFrame()
{
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GLContext::endFrame()
{
    ImGui::Render();

    int displayW = 0, displayH = 0;
    glfwGetFramebufferSize(m_window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }

    glfwSwapBuffers(m_window);
}
