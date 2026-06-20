#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../core/Image.h"
#include "../../core/Secret.h"

class DWTEncoder
{
public:
    explicit DWTEncoder(int levels = 1);

    size_t get_capacity(const Image &input) const;

    void encode(const Image &input,
                const SecretBytes &secret,
                const std::string &output_path);

    SecretBytes decode(const std::string &input_path);

private:
    int levels;

    static std::vector<uint8_t> image_to_rgb(const Image &input);

    // 2D Haar wavelet transform on a single channel
    static void haar_2d_forward(std::vector<double> &data, int width, int height, int levels);
    static void haar_2d_inverse(std::vector<double> &data, int width, int height, int levels);

    // Pack/unpack secret to/from bit vector
    static std::vector<uint8_t> pack_message(const SecretBytes &secret);
    static SecretBytes unpack_message(const std::vector<uint8_t> &bits);

    static void append_u32_bits(std::vector<uint8_t> &bits, uint32_t value);
    static uint32_t read_u32_from_bits(const std::vector<uint8_t> &bits, size_t bit_offset);
};