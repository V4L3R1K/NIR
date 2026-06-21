#include "LSBEncoder.h"

size_t LSBEncoder::get_capacity(const Image &input)
{
    return input.width * input.height * input.channels * LSB_count / 8;
}

Image LSBEncoder::encode(const Image &input, const SecretBytes &secret)
{
    size_t capacity = get_capacity(input);

    if (secret.size() != capacity)
        throw std::runtime_error("Secret size does not match capacity (call Secret::prepare first)");

    Image out = input;

    const size_t total_bits = secret.size() * 8;
    size_t bit_index = 0;

    for (size_t i = 0; i < out.data.size() && bit_index < total_bits; i++)
    {
        uint8_t &pixel_byte = out.data[i];

        pixel_byte &= ~((1 << LSB_count) - 1);

        uint8_t bits_to_write = 0;

        for (uint8_t b = 0; b < LSB_count; b++)
        {
            if (bit_index >= total_bits)
                break;

            size_t byte_index = bit_index / 8;

            uint8_t bit_in_byte = 7 - (bit_index % 8);
            uint8_t bit = (secret[byte_index] >> bit_in_byte) & 1;

            bits_to_write |= (bit << (LSB_count - b - 1));

            bit_index++;
        }

        pixel_byte |= bits_to_write;
    }

    return out;
}

SecretBytes LSBEncoder::decode(const Image &input)
{
    size_t capacity = get_capacity(input);
    const size_t total_bits = capacity * 8;
    const size_t total_pixels = input.data.size();

    SecretBytes result(capacity, 0);

    size_t bit_index = 0;

    for (size_t i = 0; i < total_pixels && bit_index < total_bits; i++)
    {
        uint8_t pixel_byte = input.data[i];

        uint8_t extracted_bits = pixel_byte & ((1 << LSB_count) - 1);

        for (int b = LSB_count - 1; b >= 0; b--)
        {
            if (bit_index >= total_bits)
                break;

            uint8_t bit = (extracted_bits >> b) & 1;
            size_t byte_index = bit_index / 8;
            uint8_t bit_in_byte = 7 - (bit_index % 8);

            result[byte_index] |= (bit << bit_in_byte);
            bit_index++;
        }
    }

    return result;
}