#include "Image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../../third_party/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../../third_party/stb_image.h"

void Image::load(const std::string &path)
{
    int loaded_channels = 0;
    unsigned char *raw = stbi_load(path.c_str(), &(this->width), &(this->height), &loaded_channels, 3);
    if (!raw)
        throw std::runtime_error("Failed to load image");

    this->channels = 3;
    this->data.assign(raw, raw + this->width * this->height * 3);

    stbi_image_free(raw);
}

void Image::savePNG(const std::string &path)
{
    stbi_write_png(path.c_str(),
                   this->width,
                   this->height,
                   this->channels,
                   this->data.data(),
                   this->width * this->channels);
}