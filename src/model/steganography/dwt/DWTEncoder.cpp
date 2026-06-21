#include "DWTEncoder.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "stb_image.h"
#include "stb_image_write.h"

DWTEncoder::DWTEncoder(int levels)
    : levels(levels)
{
    if (levels < 1)
    {
        throw std::runtime_error("DWT levels must be >= 1");
    }
}

std::vector<uint8_t> DWTEncoder::image_to_rgb(const Image &input)
{
    if (input.width <= 0 || input.height <= 0)
    {
        throw std::runtime_error("Invalid image dimensions");
    }

    if (input.data.empty())
    {
        throw std::runtime_error("Image data is empty");
    }

    std::vector<uint8_t> rgb;
    rgb.reserve(static_cast<size_t>(input.width) * static_cast<size_t>(input.height) * 3);

    if (input.channels == 3)
    {
        rgb = input.data;
        return rgb;
    }

    if (input.channels == 4)
    {
        for (size_t i = 0; i + 3 < input.data.size(); i += 4)
        {
            rgb.push_back(input.data[i + 0]);
            rgb.push_back(input.data[i + 1]);
            rgb.push_back(input.data[i + 2]);
        }
        return rgb;
    }

    if (input.channels == 1)
    {
        for (uint8_t v : input.data)
        {
            rgb.push_back(v);
            rgb.push_back(v);
            rgb.push_back(v);
        }
        return rgb;
    }

    throw std::runtime_error("Unsupported channel count (expected 1, 3, or 4)");
}

// ========== Haar 2D Forward ==========
void DWTEncoder::haar_2d_forward(std::vector<double> &data, int width, int height, int levels)
{
    int current_w = width;
    int current_h = height;

    for (int level = 0; level < levels; ++level)
    {
        // Process rows
        for (int y = 0; y < current_h; ++y)
        {
            for (int x = 0; x < current_w; x += 2)
            {
                if (x + 1 >= current_w)
                    continue;
                double a = data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
                double b = data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x + 1)];
                double avg = (a + b) / 2.0;
                double diff = (a - b) / 2.0;
                data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = avg;
                data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x + 1)] = diff;
            }
        }

        // Process columns
        for (int x = 0; x < current_w; ++x)
        {
            for (int y = 0; y < current_h; y += 2)
            {
                if (y + 1 >= current_h)
                    continue;
                double a = data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
                double b = data[static_cast<size_t>(y + 1) * static_cast<size_t>(width) + static_cast<size_t>(x)];
                double avg = (a + b) / 2.0;
                double diff = (a - b) / 2.0;
                data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = avg;
                data[static_cast<size_t>(y + 1) * static_cast<size_t>(width) + static_cast<size_t>(x)] = diff;
            }
        }

        current_w /= 2;
        current_h /= 2;
    }
}

// ========== Haar 2D Inverse ==========
void DWTEncoder::haar_2d_inverse(std::vector<double> &data, int width, int height, int levels)
{
    // We need to reconstruct starting from the smallest level
    int current_w = width / (1 << (levels - 1));
    int current_h = height / (1 << (levels - 1));

    for (int level = levels - 1; level >= 0; --level)
    {
        // Process columns (inverse)
        for (int x = 0; x < current_w; ++x)
        {
            for (int y = 0; y < current_h; y += 2)
            {
                if (y + 1 >= current_h)
                    continue;
                double avg = data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
                double diff = data[static_cast<size_t>(y + 1) * static_cast<size_t>(width) + static_cast<size_t>(x)];
                double a = avg + diff;
                double b = avg - diff;
                data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = a;
                data[static_cast<size_t>(y + 1) * static_cast<size_t>(width) + static_cast<size_t>(x)] = b;
            }
        }

        // Process rows (inverse)
        for (int y = 0; y < current_h; ++y)
        {
            for (int x = 0; x < current_w; x += 2)
            {
                if (x + 1 >= current_w)
                    continue;
                double avg = data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
                double diff = data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x + 1)];
                double a = avg + diff;
                double b = avg - diff;
                data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)] = a;
                data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x + 1)] = b;
            }
        }

        current_w *= 2;
        current_h *= 2;
    }
}

// ========== get_capacity ==========
size_t DWTEncoder::get_capacity(const Image &input) const
{
    std::vector<uint8_t> rgb = image_to_rgb(input);
    return rgb.size() / 8; // 1 bit per byte
}

// ========== encode ==========
void DWTEncoder::encode(const Image &input,
                        const SecretBytes &secret,
                        const std::string &output_path)
{
    std::vector<uint8_t> rgb = image_to_rgb(input);

    size_t total_bits_needed = secret.size() * 8;
    size_t total_bits_available = rgb.size();

    if (total_bits_needed > total_bits_available)
    {
        throw std::runtime_error("Secret is too large for this image");
    }

    // Embed into LSB of each RGB byte
    for (size_t i = 0; i < total_bits_needed; ++i)
    {
        size_t byte_idx = i / 8;
        uint8_t bit_in_byte = i % 8;
        uint8_t desired_bit = (secret[byte_idx] >> bit_in_byte) & 1U;
        rgb[i] = (rgb[i] & 0xFE) | desired_bit;
    }

    // Save as PNG
    Image out;
    out.width = input.width;
    out.height = input.height;
    out.channels = 3;
    out.data = std::move(rgb);

    out.savePNG(output_path);
}

// ========== decode ==========
SecretBytes DWTEncoder::decode(const std::string &input_path)
{
    Image img;
    img.load(input_path);

    std::vector<uint8_t> rgb = image_to_rgb(img);
    size_t total_bytes = rgb.size();

    size_t capacity = total_bytes / 8;
    size_t total_bits_needed = capacity * 8;

    SecretBytes result(capacity, 0);
    for (size_t i = 0; i < total_bits_needed && i < total_bytes; ++i)
    {
        size_t byte_idx = i / 8;
        uint8_t bit_in_byte = i % 8;
        uint8_t bit = rgb[i] & 1U;
        result[byte_idx] |= (bit << bit_in_byte);
    }

    return result;
}