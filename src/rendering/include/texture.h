#pragma once

#include <glad/glad.h>

class Texture
{
public:
    Texture();
    ~Texture();

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool create(int width, int height, const unsigned char* data = nullptr);
    void update(const unsigned char* data);
    void bind(int unit = 0) const;
    void unbind() const;

    GLuint id() const { return m_id; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool valid() const { return m_id != 0; }

private:
    void release();

    GLuint m_id = 0;
    int m_width = 0;
    int m_height = 0;
    GLenum m_internalFormat = GL_RGBA8;
    GLenum m_format = GL_RGBA;
};
