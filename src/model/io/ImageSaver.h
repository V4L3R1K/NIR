#pragma once
#include <string>
#include "../core/Image.h"

class ImageSaver {
public:
    static void savePNG(const Image& img, const std::string& path);
};