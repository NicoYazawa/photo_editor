#include "image_panel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>

ImagePanel::ImagePanel(const std::string& title)
    : m_title(title)
{
}

void ImagePanel::render(GLuint textureId, int texWidth, int texHeight)
{
    ImGui::Begin(m_title.c_str());

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 1.0f) avail.x = 1.0f;
    if (avail.y < 1.0f) avail.y = 1.0f;

    // Auto-fit on first render or when texture changes
    if (m_firstFit && texWidth > 0 && texHeight > 0)
    {
        float fitX = avail.x / static_cast<float>(texWidth);
        float fitY = avail.y / static_cast<float>(texHeight);
        m_zoom = std::min(fitX, fitY);
        m_targetZoom = m_zoom;
        m_panX = 0.0f;
        m_panY = 0.0f;
        m_firstFit = false;
    }

    handleInput();

    // Smooth zoom
    m_zoom += (m_targetZoom - m_zoom) * 0.3f;
    if (std::abs(m_targetZoom - m_zoom) < 0.001f)
        m_zoom = m_targetZoom;

    // Calculate displayed image size and position
    float displayW = texWidth * m_zoom;
    float displayH = texHeight * m_zoom;

    // Center and apply pan
    float offsetX = (avail.x - displayW) * 0.5f + m_panX;
    float offsetY = (avail.y - displayH) * 0.5f + m_panY;

    // Clamp pan so image doesn't go completely off-screen
    float minOffsetX = -(displayW * 0.8f);
    float maxOffsetX = avail.x + displayW * 0.8f - displayW;
    float minOffsetY = -(displayH * 0.8f);
    float maxOffsetY = avail.y + displayH * 0.8f - displayH;

    offsetX = std::clamp(offsetX, std::min(minOffsetX, maxOffsetX), std::max(minOffsetX, maxOffsetX));
    offsetY = std::clamp(offsetY, std::min(minOffsetY, maxOffsetY), std::max(minOffsetY, maxOffsetY));

    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    cursorPos.x += offsetX;
    cursorPos.y += offsetY;
    ImGui::SetCursorScreenPos(cursorPos);

    // Draw the image
    if (textureId && texWidth > 0 && texHeight > 0)
    {
        ImGui::Image((ImTextureID)(intptr_t)textureId,
                     ImVec2(displayW, displayH));
    }
    else
    {
        // Placeholder text
        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x - offsetX + avail.x * 0.3f,
                                          cursorPos.y - offsetY + avail.y * 0.5f));
        ImGui::TextDisabled("No image loaded");
    }

    // Overlay info
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (texWidth > 0)
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "%dx%d | %.0f%%", texWidth, texHeight, m_zoom * 100.0f);
        ImVec2 textPos = ImVec2(window->Pos.x + 8, window->Pos.y + window->Size.y - 24);
        ImGui::GetForegroundDrawList()->AddText(textPos, IM_COL32(255,255,255,180), buf);
    }

    ImGui::End();
}

void ImagePanel::handleInput()
{
    if (!ImGui::IsWindowHovered())
        return;

    // Zoom with mouse wheel
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f)
    {
        float factor = (wheel > 0) ? 1.1f : 0.90909f;
        m_targetZoom = std::clamp(m_targetZoom * factor, m_minZoom, m_maxZoom);
    }

    // Pan with middle mouse button or right mouse button
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
        if (delta.x == 0 && delta.y == 0)
            delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);

        m_panX += delta.x;
        m_panY += delta.y;

        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
    }
}

void ImagePanel::resetView()
{
    m_firstFit = true;
    m_panX = 0.0f;
    m_panY = 0.0f;
}
