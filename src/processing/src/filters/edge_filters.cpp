#include "filters/edge_filters.h"
#include "shader.h"

#include <imgui.h>

// ============ Sobel ============
static const char* kSobelFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform vec2 uTexelSize;

void main() {
    // Sample 3x3 neighborhood (BT.709 luminance weights)
    float tl = dot(texture(uTexture, vTexCoord + vec2(-1,-1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float tc = dot(texture(uTexture, vTexCoord + vec2( 0,-1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float tr = dot(texture(uTexture, vTexCoord + vec2( 1,-1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float ml = dot(texture(uTexture, vTexCoord + vec2(-1, 0)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float mr = dot(texture(uTexture, vTexCoord + vec2( 1, 0)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float bl = dot(texture(uTexture, vTexCoord + vec2(-1, 1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float bc = dot(texture(uTexture, vTexCoord + vec2( 0, 1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float br = dot(texture(uTexture, vTexCoord + vec2( 1, 1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));

    float gx = -tl + tr - 2.0*ml + 2.0*mr - bl + br;
    float gy = -tl - 2.0*tc - tr + bl + 2.0*bc + br;
    float mag = sqrt(gx*gx + gy*gy);

    FragColor = vec4(vec3(mag), 1.0);
}
)";
const char* SobelFilter::fragmentShaderSource() const { return kSobelFrag; }
std::unique_ptr<FilterBase> SobelFilter::clone() const {
    return std::make_unique<SobelFilter>(*this);
}

// ============ Laplacian ============
static const char* kLaplacianFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform vec2 uTexelSize;

void main() {
    float c  = dot(texture(uTexture, vTexCoord).rgb, vec3(0.299, 0.587, 0.114));
    float n  = dot(texture(uTexture, vTexCoord + vec2(0,-1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float s  = dot(texture(uTexture, vTexCoord + vec2(0, 1)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float w  = dot(texture(uTexture, vTexCoord + vec2(-1,0)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));
    float e  = dot(texture(uTexture, vTexCoord + vec2(1, 0)*uTexelSize).rgb, vec3(0.299, 0.587, 0.114));

    float laplace = abs(4.0 * c - n - s - w - e);
    FragColor = vec4(vec3(laplace), 1.0);
}
)";
const char* LaplacianFilter::fragmentShaderSource() const { return kLaplacianFrag; }
std::unique_ptr<FilterBase> LaplacianFilter::clone() const {
    return std::make_unique<LaplacianFilter>(*this);
}

// ============ Sharpen (Unsharp Mask) ============
// This is a single-pass approximation using a 3x3 sharpen kernel
static const char* kSharpenFrag = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform vec2 uTexelSize;
uniform float uStrength;

void main() {
    vec4 c  = texture(uTexture, vTexCoord);
    vec4 n  = texture(uTexture, vTexCoord + vec2(0,-1)*uTexelSize);
    vec4 s  = texture(uTexture, vTexCoord + vec2(0, 1)*uTexelSize);
    vec4 w  = texture(uTexture, vTexCoord + vec2(-1,0)*uTexelSize);
    vec4 e  = texture(uTexture, vTexCoord + vec2(1, 0)*uTexelSize);

    vec4 blur = 0.25 * (n + s + w + e);
    vec4 result = c + (c - blur) * uStrength;
    FragColor = vec4(clamp(result.rgb, 0.0, 1.0), c.a);
}
)";
const char* SharpenFilter::fragmentShaderSource() const { return kSharpenFrag; }
void SharpenFilter::setUniforms(Shader& shader) {
    shader.setFloat("uStrength", strength);
}
void SharpenFilter::renderConfigUI() {
    ImGui::SliderFloat("Strength", &strength, 0.0f, 3.0f);
}
std::unique_ptr<FilterBase> SharpenFilter::clone() const {
    return std::make_unique<SharpenFilter>(*this);
}
