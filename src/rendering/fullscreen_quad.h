#pragma once

#include <glad/glad.h>

class FullscreenQuad
{
public:
    FullscreenQuad();
    ~FullscreenQuad();

    FullscreenQuad(const FullscreenQuad&) = delete;
    FullscreenQuad& operator=(const FullscreenQuad&) = delete;

    void draw() const;
    bool valid() const { return m_vao != 0; }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};
