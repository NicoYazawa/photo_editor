#pragma once
#include "../filter_base.h"

class GaussianBlurFilter : public FilterBase
{
public:
    std::string name() const override { return "Gaussian Blur"; }
    std::string category() const override { return "Blur"; }
    std::string key() const override { return "blur.gaussian"; }
    const char* fragmentShaderSource() const override;
    int passCount() const override { return 2; }
    void setUniformsForPass(class Shader& shader, int pass) override;
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    std::unique_ptr<FilterBase> clone() const override;

    int kernelSize = 5;   // must be odd, clamped in UI
    float sigma = 2.0f;
};

class BoxBlurFilter : public FilterBase
{
public:
    std::string name() const override { return "Box Blur"; }
    std::string category() const override { return "Blur"; }
    std::string key() const override { return "blur.box"; }
    const char* fragmentShaderSource() const override;
    int passCount() const override { return 2; }
    void setUniformsForPass(class Shader& shader, int pass) override;
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    std::unique_ptr<FilterBase> clone() const override;

    int radius = 3;
};
