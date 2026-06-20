#pragma once

#include <glad/glad.h>

class FBO
{
public:
    FBO() = default;
    ~FBO();

    FBO(FBO&& other) noexcept;
    FBO& operator=(FBO&& other) noexcept;

    FBO(const FBO&) = delete;
    FBO& operator=(const FBO&) = delete;

    bool create(int width, int height);
    void bind() const;
    void unbind() const;

    GLuint fboId() const { return m_fbo; }
    GLuint colorTexture() const { return m_colorTexture; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool valid() const { return m_fbo != 0; }

private:
    void release();

    GLuint m_fbo = 0;
    GLuint m_colorTexture = 0;
    int m_width = 0;
    int m_height = 0;
};
