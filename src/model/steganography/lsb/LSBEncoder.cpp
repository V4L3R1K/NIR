#include "LSBEncoder.h"

size_t LSBEncoder::get_capacity(const Image &input)
{
    return input.width * input.height * input.channels * LSB_count / 8;
}

Image LSBEncoder::encode(const Image &input, const SecretBytes &secret)
{
    uint32_t secret_size = static_cast<uint32_t>(secret.size());
    size_t total_bytes_to_store = sizeof(uint32_t) + secret.size();

    if (get_capacity(input) < total_bytes_to_store)
        throw std::runtime_error("Secret is too large for this image");

    Image out = input;

    std::vector<uint8_t> payload;
    payload.reserve(total_bytes_to_store);

    payload.push_back((secret_size >> 0) & 0xFF);
    payload.push_back((secret_size >> 8) & 0xFF);
    payload.push_back((secret_size >> 16) & 0xFF);
    payload.push_back((secret_size >> 24) & 0xFF);

    payload.insert(payload.end(), secret.begin(), secret.end());

    const size_t total_bits = payload.size() * 8;
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
            uint8_t bit = (payload[byte_index] >> bit_in_byte) & 1;

            bits_to_write |= (bit << (LSB_count - b - 1));

            bit_index++;
        }

        pixel_byte |= bits_to_write;
    }

    return out;
}

SecretBytes LSBEncoder::decode(const Image &input)
{
    const size_t total_pixels = input.data.size();

    std::vector<uint8_t> extracted_bytes;
    extracted_bytes.reserve(1024);

    uint8_t current_byte = 0;
    uint8_t bits_collected = 0;

    uint32_t secret_size = 0;
    bool size_read = false;

    size_t byte_index = 0;

    for (size_t i = 0; i < total_pixels; i++)
    {
        uint8_t pixel_byte = input.data[i];

        uint8_t extracted_bits = pixel_byte & ((1 << LSB_count) - 1);

        for (int b = LSB_count - 1; b >= 0; b--)
        {
            uint8_t bit = (extracted_bits >> b) & 1;

            current_byte = (current_byte << 1) | bit;
            bits_collected++;

            if (bits_collected == 8)
            {
                extracted_bytes.push_back(current_byte);

                current_byte = 0;
                bits_collected = 0;

                if (!size_read && extracted_bytes.size() == sizeof(uint32_t))
                {
                    secret_size =
                        (static_cast<uint32_t>(extracted_bytes[0]) << 0) |
                        (static_cast<uint32_t>(extracted_bytes[1]) << 8) |
                        (static_cast<uint32_t>(extracted_bytes[2]) << 16) |
                        (static_cast<uint32_t>(extracted_bytes[3]) << 24);

                    size_read = true;

                    extracted_bytes.reserve(sizeof(uint32_t) + secret_size);
                }

                if (size_read &&
                    extracted_bytes.size() == sizeof(uint32_t) + secret_size)
                {
                    SecretBytes result(
                        extracted_bytes.begin() + sizeof(uint32_t),
                        extracted_bytes.end());

                    return result;
                }
            }
        }
    }

    throw std::runtime_error("Failed to decode secret");
}