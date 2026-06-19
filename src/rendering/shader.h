#pragma once

#include <glad/glad.h>

#include <string>
#include <unordered_map>

class Shader
{
public:
    Shader() = default;
    Shader(const char* vertSrc, const char* fragSrc);
    Shader(const char* vertSrc, const char* geomSrc, const char* fragSrc);
    ~Shader();

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool compile(const char* vertSrc, const char* fragSrc);
    bool valid() const { return m_program != 0; }

    void use() const;
    GLuint program() const { return m_program; }

    // Uniform setters
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;

    // Build from external file sources (loads file content via callback)
    using SourceLoader = std::string (*)(const std::string& path);
    static Shader fromFiles(const std::string& vertPath,
                            const std::string& fragPath,
                            SourceLoader loader);

private:
    GLuint compileShader(GLenum type, const char* source);
    GLint getUniformLocation(const char* name) const;
    void release();

    GLuint m_program = 0;
    mutable std::unordered_map<std::string, GLint> m_uniformCache;
};
