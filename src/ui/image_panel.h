#pragma once

#include <string>

#include <glad/glad.h>

class ImagePanel
{
public:
    ImagePanel(const std::string& title);
    ~ImagePanel() = default;

    void render(GLuint textureId, int texWidth, int texHeight);
    void resetView();

    // Zoom and pan state
    float zoom() const { return m_zoom; }
    void setTitle(const std::string& title) { m_title = title; }

private:
    void handleInput();

    std::string m_title;
    float m_zoom = 1.0f;
    float m_panX = 0.0f;   // in image pixel space
    float m_panY = 0.0f;
    float m_targetZoom = 1.0f;

    bool m_firstFit = true;
    float m_minZoom = 0.05f;
    float m_maxZoom = 20.0f;
};
