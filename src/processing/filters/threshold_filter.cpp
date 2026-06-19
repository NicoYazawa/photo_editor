#include "threshold_filter.h"
#include "../../rendering/shader.h"

#include <imgui.h>

static const char* kThresholdFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uThreshold;
uniform float uSoftness;

void main() {
    vec4 c = texture(uTexture, vTexCoord);
    float lum = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    float t = smoothstep(uThreshold - uSoftness, uThreshold + uSoftness, lum);
    FragColor = vec4(vec3(t), c.a);
}
)";

const char* ThresholdFilter::fragmentShaderSource() const { return kThresholdFrag; }

void ThresholdFilter::setUniforms(Shader& shader)
{
    shader.setFloat("uThreshold", threshold);
    shader.setFloat("uSoftness", softness);
}

void ThresholdFilter::renderConfigUI()
{
    ImGui::SliderFloat("Threshold", &threshold, 0.0f, 1.0f);
    ImGui::SliderFloat("Softness", &softness, 0.0f, 0.5f);
}

std::unique_ptr<FilterBase> ThresholdFilter::clone() const
{
    return std::make_unique<ThresholdFilter>(*this);
}
