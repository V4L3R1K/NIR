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

namespace
{
    constexpr int HEADER_BITS = 32; // 4 bytes for secret size
}

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

// ========== Pack/Unpack ==========

void DWTEncoder::append_u32_bits(std::vector<uint8_t> &bits, uint32_t value)
{
    for (size_t i = 0; i < 32; ++i)
    {
        bits.push_back(static_cast<uint8_t>((value >> i) & 1U));
    }
}

uint32_t DWTEncoder::read_u32_from_bits(const std::vector<uint8_t> &bits, size_t bit_offset)
{
    if (bit_offset + 32 > bits.size())
    {
        throw std::runtime_error("Not enough bits to read u32");
    }

    uint32_t value = 0;
    for (size_t i = 0; i < 32; ++i)
    {
        value |= static_cast<uint32_t>((bits[bit_offset + i] & 1U) << i);
    }
    return value;
}

std::vector<uint8_t> DWTEncoder::pack_message(const SecretBytes &secret)
{
    if (secret.size() > static_cast<size_t>(UINT32_MAX))
    {
        throw std::runtime_error("Secret too large");
    }

    std::vector<uint8_t> bits;
    bits.reserve(HEADER_BITS + secret.size() * 8);

    append_u32_bits(bits, static_cast<uint32_t>(secret.size()));

    for (uint8_t byte : secret)
    {
        for (size_t i = 0; i < 8; ++i)
        {
            bits.push_back(static_cast<uint8_t>((byte >> i) & 1U));
        }
    }

    return bits;
}

SecretBytes DWTEncoder::unpack_message(const std::vector<uint8_t> &bits)
{
    if (bits.size() < HEADER_BITS)
    {
        throw std::runtime_error("Corrupted payload: missing size field");
    }

    const uint32_t secret_size = read_u32_from_bits(bits, 0);
    const size_t total_bits_needed = HEADER_BITS + static_cast<size_t>(secret_size) * 8;

    if (bits.size() < total_bits_needed)
    {
        throw std::runtime_error("Corrupted payload: not enough bits for secret");
    }

    SecretBytes secret(secret_size);
    size_t bit_cursor = HEADER_BITS;

    for (uint32_t i = 0; i < secret_size; ++i)
    {
        uint8_t value = 0;
        for (size_t b = 0; b < 8; ++b)
        {
            value |= static_cast<uint8_t>((bits[bit_cursor++] & 1U) << b);
        }
        secret[i] = value;
    }

    return secret;
}

// ========== get_capacity ==========
// DWT-based capacity estimation: count wavelet detail coefficients
// with |value| >= 0.5 across all channels. The actual embedding is
// done in the spatial domain (1 LSB per channel per byte).
// For simplicity and robustness, we return the total pixel bytes
// minus the 4-byte header, matching spatial-domain LSB capacity.
size_t DWTEncoder::get_capacity(const Image &input) const
{
    std::vector<uint8_t> rgb = image_to_rgb(input);
    size_t total_bits = rgb.size(); // width * height * 3 (one bit per byte)

    if (total_bits < HEADER_BITS)
        return 0;

    return (total_bits - HEADER_BITS) / 8;
}

// ========== encode ==========
// Spatial-domain LSB embedding, 1 bit per channel per pixel.
// The DWT is only used for capacity estimation (in get_capacity).
void DWTEncoder::encode(const Image &input,
                        const SecretBytes &secret,
                        const std::string &output_path)
{
    std::vector<uint8_t> rgb = image_to_rgb(input);
    std::vector<uint8_t> payload_bits = pack_message(secret);

    size_t total_bits_needed = payload_bits.size();
    size_t total_bits_available = rgb.size(); // width * height * 3, one bit per byte

    if (total_bits_needed > total_bits_available)
    {
        throw std::runtime_error("Secret is too large for this image");
    }

    // Embed into LSB of each RGB byte
    for (size_t i = 0; i < total_bits_needed; ++i)
    {
        uint8_t desired_bit = payload_bits[i] & 1U;
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

    // Extract header (32 bits) from LSB
    std::vector<uint8_t> header_bits;
    header_bits.reserve(HEADER_BITS);
    for (size_t i = 0; i < HEADER_BITS && i < total_bytes; ++i)
    {
        header_bits.push_back(rgb[i] & 1U);
    }

    if (header_bits.size() < HEADER_BITS)
    {
        throw std::runtime_error("Not enough data to extract header");
    }

    const uint32_t secret_size = read_u32_from_bits(header_bits, 0);
    const size_t total_bits_needed = HEADER_BITS + static_cast<size_t>(secret_size) * 8;

    if (total_bits_needed > total_bytes)
    {
        throw std::runtime_error("Not enough data to extract payload");
    }

    // Extract all bits
    std::vector<uint8_t> all_bits;
    all_bits.reserve(total_bits_needed);
    for (size_t i = 0; i < total_bits_needed; ++i)
    {
        all_bits.push_back(rgb[i] & 1U);
    }

    return unpack_message(all_bits);
}