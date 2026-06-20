#include "filters/blur_filters.h"
#include "shader.h"

#include <imgui.h>
#include <cmath>
#include <vector>

// Single separable blur shader: controlled by uHorizontal uniform
// Uses compile-time weights (passed via uniform array or computed in-shader)
static const char* kGaussianBlurFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform vec2 uTexelSize;
uniform int uHorizontal;

// Gaussian weights for kernel radius 15 (max), computed for sigma=2.0 default
// Will be dynamically adjusted in the shader for different sigmas
uniform float uWeights[15];
uniform int uKernelRadius;

void main() {
    vec2 dir = (uHorizontal == 1) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 result = vec4(0.0);
    float weightSum = 0.0;
    for (int i = -uKernelRadius; i <= uKernelRadius; ++i) {
        float w = uWeights[abs(i)];
        result += texture(uTexture, vTexCoord + dir * uTexelSize * float(i)) * w;
        weightSum += w;
    }
    FragColor = result / weightSum;
}
)";

// Box blur: equal weights
static const char* kBoxBlurFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform vec2 uTexelSize;
uniform int uHorizontal;
uniform int uRadius;

void main() {
    vec2 dir = (uHorizontal == 1) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 result = vec4(0.0);
    float count = 0.0;
    for (int i = -uRadius; i <= uRadius; ++i) {
        result += texture(uTexture, vTexCoord + dir * uTexelSize * float(i));
        count += 1.0;
    }
    FragColor = result / count;
}
)";

static std::vector<float> computeGaussianWeights(int radius, float sigma)
{
    std::vector<float> weights(radius + 1);
    float sum = 0.0f;
    for (int i = 0; i <= radius; ++i)
    {
        float x = static_cast<float>(i);
        weights[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += (i == 0) ? weights[i] : 2.0f * weights[i];
    }
    for (int i = 0; i <= radius; ++i)
        weights[i] /= sum;
    return weights;
}

// ============ Gaussian Blur ============
const char* GaussianBlurFilter::fragmentShaderSource() const { return kGaussianBlurFrag; }

void GaussianBlurFilter::setUniformsForPass(Shader& shader, int pass)
{
    shader.setInt("uHorizontal", pass == 0 ? 1 : 0);

    int radius = std::min(kernelSize / 2, 14); // max 14 for 15-element array
    shader.setInt("uKernelRadius", radius);

    auto weights = computeGaussianWeights(radius, sigma);
    for (int i = 0; i <= radius; ++i)
    {
        char name[32];
        snprintf(name, sizeof(name), "uWeights[%d]", i);
        glUniform1f(glGetUniformLocation(shader.program(), name), weights[i]);
    }
}

void GaussianBlurFilter::renderConfigUI()
{
    int k = kernelSize;
    ImGui::SliderInt("Kernel Size", &k, 1, 15);
    if (k % 2 == 0) k++; // ensure odd
    kernelSize = k;
    ImGui::SliderFloat("Sigma", &sigma, 0.5f, 10.0f);
}

std::unique_ptr<FilterBase> GaussianBlurFilter::clone() const {
    return std::make_unique<GaussianBlurFilter>(*this);
}

// ============ Box Blur ============
const char* BoxBlurFilter::fragmentShaderSource() const { return kBoxBlurFrag; }

void BoxBlurFilter::setUniformsForPass(Shader& shader, int pass)
{
    shader.setInt("uHorizontal", pass == 0 ? 1 : 0);
    shader.setInt("uRadius", radius);
}

void BoxBlurFilter::renderConfigUI()
{
    ImGui::SliderInt("Radius", &radius, 1, 20);
}

std::unique_ptr<FilterBase> BoxBlurFilter::clone() const {
    return std::make_unique<BoxBlurFilter>(*this);
}
