#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>

class Image
{
public:
    int width;
    int height;
    int channels;

    std::vector<uint8_t> data;

    void load(const std::string &path);
    void savePNG(const std::string &path);
};