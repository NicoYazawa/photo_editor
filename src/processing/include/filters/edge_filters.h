#pragma once
#include "filter_base.h"

class SobelFilter : public FilterBase
{
public:
    std::string name() const override { return "Sobel Edge"; }
    std::string category() const override { return "Edge Detect"; }
    std::string key() const override { return "edge.sobel"; }
    const char* fragmentShaderSource() const override;
    std::unique_ptr<FilterBase> clone() const override;
};

class LaplacianFilter : public FilterBase
{
public:
    std::string name() const override { return "Laplacian"; }
    std::string category() const override { return "Edge Detect"; }
    std::string key() const override { return "edge.laplacian"; }
    const char* fragmentShaderSource() const override;
    std::unique_ptr<FilterBase> clone() const override;
};

class SharpenFilter : public FilterBase
{
public:
    std::string name() const override { return "Sharpen"; }
    std::string category() const override { return "Enhance"; }
    std::string key() const override { return "enhance.sharpen"; }
    const char* fragmentShaderSource() const override;
    void setUniforms(class Shader& shader) override;
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    std::unique_ptr<FilterBase> clone() const override;

    float strength = 1.0f; // [0.0, 3.0]
};
