#include "image_io.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <glad/glad.h>
#include <cstdio>
#include <cstdlib>

namespace ImageIO
{

ImageData load(const std::string& path)
{
    ImageData result;

    int w = 0, h = 0, c = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4); // force RGBA
    if (!data)
    {
        fprintf(stderr, "[ImageIO] Failed to load: %s\n  Reason: %s\n",
                path.c_str(), stbi_failure_reason());
        return result;
    }

    printf("[ImageIO] Loaded: %s (%dx%d, %d channels)\n", path.c_str(), w, h, c);

    result.width = w;
    result.height = h;
    result.channels = c;
    result.pixels.assign(data, data + (w * h * 4));

    stbi_image_free(data);
    return result;
}

bool save(const std::string& path,
          const unsigned char* rgbaData,
          int width, int height)
{
    if (!rgbaData || width <= 0 || height <= 0)
        return false;

    // Determine format from extension
    std::string ext;
    auto dotPos = path.rfind('.');
    if (dotPos != std::string::npos)
        ext = path.substr(dotPos + 1);

    int result = 0;
    if (ext == "jpg" || ext == "jpeg")
    {
        result = stbi_write_jpg(path.c_str(), width, height, 4, rgbaData, 90);
    }
    else
    {
        result = stbi_write_png(path.c_str(), width, height, 4, rgbaData, width * 4);
    }

    if (!result)
        fprintf(stderr, "[ImageIO] Failed to save: %s\n", path.c_str());

    return result != 0;
}

std::vector<unsigned char> readFramebuffer(int x, int y, int width, int height)
{
    std::vector<unsigned char> pixels(width * height * 4);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}

} // namespace ImageIO
