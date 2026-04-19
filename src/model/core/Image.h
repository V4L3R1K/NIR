#pragma once
#include <vector>
#include <cstdint>

class Image {
public:
    int width;
    int height;
    int channels;

    std::vector<uint8_t> data;
};