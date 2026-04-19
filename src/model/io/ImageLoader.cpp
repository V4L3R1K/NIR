#include "ImageLoader.h"
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "../../../third_party/stb_image.h"

Image ImageLoader::load(const std::string& path) {
    int w, h, c;

    unsigned char* raw = stbi_load(path.c_str(), &w, &h, &c, 3);
    if (!raw) {
        throw std::runtime_error("Failed to load image");
    }

    Image img;
    img.width = w;
    img.height = h;
    img.channels = 3;

    img.data.assign(raw, raw + w * h * 3);

    stbi_image_free(raw);
    return img;
}