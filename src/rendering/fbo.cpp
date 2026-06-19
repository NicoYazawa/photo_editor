#include "fbo.h"

#include <cstdio>

FBO::~FBO()
{
    release();
}

FBO::FBO(FBO&& other) noexcept
    : m_fbo(other.m_fbo)
    , m_colorTexture(other.m_colorTexture)
    , m_width(other.m_width)
    , m_height(other.m_height)
{
    other.m_fbo = 0;
    other.m_colorTexture = 0;
    other.m_width = 0;
    other.m_height = 0;
}

FBO& FBO::operator=(FBO&& other) noexcept
{
    if (this != &other)
    {
        release();
        m_fbo = other.m_fbo;
        m_colorTexture = other.m_colorTexture;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_fbo = 0;
        other.m_colorTexture = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

bool FBO::create(int width, int height)
{
    release();

    m_width = width;
    m_height = height;

    // Create color texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create FBO and attach texture
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_colorTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "[FBO] Framebuffer incomplete: 0x%x\n", status);
        release();
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void FBO::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void FBO::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::release()
{
    if (m_fbo)
    {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_colorTexture)
    {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    m_width = 0;
    m_height = 0;
}
