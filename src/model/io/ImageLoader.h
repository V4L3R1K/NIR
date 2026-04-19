#pragma once
#include <string>
#include "../core/Image.h"

class ImageLoader {
public:
    static Image load(const std::string& path);
};