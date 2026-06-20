#pragma once

#include <memory>
#include <string>

class Shader;

class FilterBase
{
public:
    virtual ~FilterBase() = default;

    // Metadata
    virtual std::string name() const = 0;
    virtual std::string category() const = 0;

    // Unique key for registry lookup (e.g. "color.grayscale")
    virtual std::string key() const { return category() + "." + name(); }

    // Shader source
    virtual const char* fragmentShaderSource() const = 0;

    // Number of passes (most filters: 1; separable blur: 2)
    virtual int passCount() const { return 1; }

    // Set uniforms before each pass
    virtual void setUniforms(Shader& shader) {}
    virtual void setUniformsForPass(Shader& shader, int pass) { setUniforms(shader); }

    // Config UI (rendered in the filter config panel)
    virtual bool hasConfigUI() const { return false; }
    virtual void renderConfigUI() {}

    // Clone for filter chain copy
    virtual std::unique_ptr<FilterBase> clone() const = 0;
};
