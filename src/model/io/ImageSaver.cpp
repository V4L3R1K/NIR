#include "ImageSaver.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../../third_party/stb_image_write.h"

void ImageSaver::savePNG(const Image& img, const std::string& path) {
    stbi_write_png(path.c_str(),
                   img.width,
                   img.height,
                   img.channels,
                   img.data.data(),
                   img.width * img.channels);
}