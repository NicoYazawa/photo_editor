#pragma once
#include "filter_base.h"

class ThresholdFilter : public FilterBase
{
public:
    std::string name() const override { return "Threshold"; }
    std::string category() const override { return "Color"; }
    std::string key() const override { return "color.threshold"; }
    const char* fragmentShaderSource() const override;
    void setUniforms(class Shader& shader) override;
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    std::unique_ptr<FilterBase> clone() const override;

private:
    float threshold = 0.5f;   // [0.0, 1.0]
    float softness = 0.0f;    // [0.0, 0.5]
};
