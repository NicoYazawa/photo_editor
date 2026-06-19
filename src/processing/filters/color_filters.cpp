#include "color_filters.h"
#include "../../rendering/shader.h"

#include <imgui.h>

// ============ Grayscale ============
static const char* kGrayFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    vec4 c = texture(uTexture, vTexCoord);
    float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(vec3(g), c.a);
}
)";
const char* GrayscaleFilter::fragmentShaderSource() const { return kGrayFrag; }
std::unique_ptr<FilterBase> GrayscaleFilter::clone() const {
    return std::make_unique<GrayscaleFilter>(*this);
}

// ============ Invert ============
static const char* kInvertFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    vec4 c = texture(uTexture, vTexCoord);
    FragColor = vec4(1.0 - c.rgb, c.a);
}
)";
const char* InvertFilter::fragmentShaderSource() const { return kInvertFrag; }
std::unique_ptr<FilterBase> InvertFilter::clone() const {
    return std::make_unique<InvertFilter>(*this);
}

// ============ Brightness/Contrast ============
static const char* kBCFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uBrightness;
uniform float uContrast;
void main() {
    vec4 c = texture(uTexture, vTexCoord);
    vec3 result = (c.rgb - 0.5) * uContrast + 0.5 + uBrightness;
    FragColor = vec4(clamp(result, 0.0, 1.0), c.a);
}
)";
const char* BrightnessContrastFilter::fragmentShaderSource() const { return kBCFrag; }
void BrightnessContrastFilter::setUniforms(Shader& shader) {
    shader.setFloat("uBrightness", brightness);
    shader.setFloat("uContrast", contrast);
}
void BrightnessContrastFilter::renderConfigUI() {
    ImGui::SliderFloat("Brightness", &brightness, -1.0f, 1.0f);
    ImGui::SliderFloat("Contrast", &contrast, 0.0f, 3.0f);
}
std::unique_ptr<FilterBase> BrightnessContrastFilter::clone() const {
    return std::make_unique<BrightnessContrastFilter>(*this);
}

// ============ Color Balance ============
static const char* kCBFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uRedShift;
uniform float uGreenShift;
uniform float uBlueShift;
void main() {
    vec4 c = texture(uTexture, vTexCoord);
    vec3 result = c.rgb + vec3(uRedShift, uGreenShift, uBlueShift);
    FragColor = vec4(clamp(result, 0.0, 1.0), c.a);
}
)";
const char* ColorBalanceFilter::fragmentShaderSource() const { return kCBFrag; }
void ColorBalanceFilter::setUniforms(Shader& shader) {
    shader.setFloat("uRedShift", redShift);
    shader.setFloat("uGreenShift", greenShift);
    shader.setFloat("uBlueShift", blueShift);
}
void ColorBalanceFilter::renderConfigUI() {
    ImGui::SliderFloat("Red Shift", &redShift, -1.0f, 1.0f);
    ImGui::SliderFloat("Green Shift", &greenShift, -1.0f, 1.0f);
    ImGui::SliderFloat("Blue Shift", &blueShift, -1.0f, 1.0f);
}
std::unique_ptr<FilterBase> ColorBalanceFilter::clone() const {
    return std::make_unique<ColorBalanceFilter>(*this);
}
