#include "shader.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

Shader::Shader(const char* vertSrc, const char* fragSrc)
{
    compile(vertSrc, fragSrc);
}

Shader::Shader(const char* vertSrc, const char* geomSrc, const char* fragSrc)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint gs = compileShader(GL_GEOMETRY_SHADER, geomSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    if (!vs || !gs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (gs) glDeleteShader(gs);
        if (fs) glDeleteShader(fs);
        return;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, gs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    GLint success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        fprintf(stderr, "[Shader] Program link error:\n%s\n", log);
        release();
    }

    glDeleteShader(vs);
    glDeleteShader(gs);
    glDeleteShader(fs);
}

Shader::~Shader()
{
    release();
}

Shader::Shader(Shader&& other) noexcept
    : m_program(other.m_program)
    , m_uniformCache(std::move(other.m_uniformCache))
{
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        release();
        m_program = other.m_program;
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_program = 0;
    }
    return *this;
}

bool Shader::compile(const char* vertSrc, const char* fragSrc)
{
    release();

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    GLint success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        fprintf(stderr, "[Shader] Program link error:\n%s\n", log);
        release();
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return m_program != 0;
}

void Shader::use() const
{
    glUseProgram(m_program);
}

GLuint Shader::compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        const char* typeStr = (type == GL_VERTEX_SHADER) ? "vertex" :
                              (type == GL_FRAGMENT_SHADER) ? "fragment" : "geometry";
        fprintf(stderr, "[Shader] %s shader compile error:\n%s\n", typeStr, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLint Shader::getUniformLocation(const char* name) const
{
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end())
        return it->second;

    GLint loc = glGetUniformLocation(m_program, name);
    m_uniformCache[name] = loc;
    return loc;
}

void Shader::setInt(const char* name, int value) const
{
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const char* name, float value) const
{
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const char* name, float x, float y) const
{
    glUniform2f(getUniformLocation(name), x, y);
}

void Shader::setVec3(const char* name, float x, float y, float z) const
{
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setVec4(const char* name, float x, float y, float z, float w) const
{
    glUniform4f(getUniformLocation(name), x, y, z, w);
}

Shader Shader::fromFiles(const std::string& vertPath,
                          const std::string& fragPath,
                          SourceLoader loader)
{
    std::string vertSrc = loader(vertPath);
    std::string fragSrc = loader(fragPath);
    return Shader(vertSrc.c_str(), fragSrc.c_str());
}

void Shader::release()
{
    if (m_program)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_uniformCache.clear();
}
