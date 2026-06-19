#pragma once

#include <string>
#include <vector>

struct ImageData
{
    std::vector<unsigned char> pixels; // RGBA
    int width = 0;
    int height = 0;
    int channels = 0; // original channel count before conversion

    bool valid() const { return !pixels.empty(); }
};

namespace ImageIO
{
    // Load image from file, always returns RGBA data
    ImageData load(const std::string& path);

    // Save RGBA data to file (PNG or JPEG determined by extension)
    bool save(const std::string& path,
              const unsigned char* rgbaData,
              int width, int height);

    // Read back framebuffer pixels to RGBA buffer
    std::vector<unsigned char> readFramebuffer(int x, int y, int width, int height);
}
