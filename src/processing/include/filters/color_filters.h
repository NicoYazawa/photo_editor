#pragma once
#include "filter_base.h"

class GrayscaleFilter : public FilterBase
{
public:
    std::string name() const override { return "Grayscale"; }
    std::string category() const override { return "Color"; }
    std::string key() const override { return "color.grayscale"; }
    const char* fragmentShaderSource() const override;
    std::unique_ptr<FilterBase> clone() const override;
};

class InvertFilter : public FilterBase
{
public:
    std::string name() const override { return "Invert"; }
    std::string category() const override { return "Color"; }
    std::string key() const override { return "color.invert"; }
    const char* fragmentShaderSource() const override;
    std::unique_ptr<FilterBase> clone() const override;
};

class BrightnessContrastFilter : public FilterBase
{
public:
    std::string name() const override { return "Brightness/Contrast"; }
    std::string category() const override { return "Color"; }
    std::string key() const override { return "color.brightness_contrast"; }
    const char* fragmentShaderSource() const override;
    void setUniforms(class Shader& shader) override;
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    std::unique_ptr<FilterBase> clone() const override;

private:
    float brightness = 0.0f;   // [-1.0, 1.0]
    float contrast = 1.0f;     // [0.0, 3.0]
};

class ColorBalanceFilter : public FilterBase
{
public:
    std::string name() const override { return "Color Balance"; }
    std::string category() const override { return "Color"; }
    std::string key() const override { return "color.color_balance"; }
    const char* fragmentShaderSource() const override;
    void setUniforms(class Shader& shader) override;
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    std::unique_ptr<FilterBase> clone() const override;

private:
    float redShift = 0.0f;    // [-1.0, 1.0]
    float greenShift = 0.0f;  // [-1.0, 1.0]
    float blueShift = 0.0f;   // [-1.0, 1.0]
};

class SaturationFilter : public FilterBase
{
public:
    std::string name() const override { return "Saturation"; }
    std::string category() const override { return "Color"; }
    std::string key() const override { return "color.saturation"; }
    const char* fragmentShaderSource() const override;
    void setUniforms(class Shader& shader) override;
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    std::unique_ptr<FilterBase> clone() const override;

private:
    float saturation = 1.5f;
};