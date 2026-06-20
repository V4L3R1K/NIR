#include "PVDEncoder.h"

#include <cmath>
#include <algorithm>

size_t PVDEncoder::get_capacity(const Image &input)
{
    const uint8_t mask = (1 << PVD_count) - 1;
    size_t usable_pairs = 0;

    for (size_t i = 0; i + 1 < input.data.size(); i += 2)
    {
        int16_t a = input.data[i];
        int16_t b = input.data[i + 1];
        int16_t avg = (a + b) / 2;

        int16_t d_max = std::min(
            static_cast<int16_t>(2 * avg),
            static_cast<int16_t>(2 * (255 - avg)));

        if (d_max >= static_cast<int16_t>(mask))
            usable_pairs++;
    }

    size_t bits = usable_pairs * PVD_count;
    return bits / 8 - 4;
}

Image PVDEncoder::encode(const Image &input, const SecretBytes &secret)
{
    uint32_t secret_size = static_cast<uint32_t>(secret.size());
    size_t total_bytes_to_store = sizeof(uint32_t) + secret.size();

    if (get_capacity(input) < secret_size)
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
    const uint8_t mask = (1 << PVD_count) - 1;

    for (size_t i = 0; i + 1 < out.data.size() && bit_index < total_bits; i += 2)
    {
        int16_t a = out.data[i];
        int16_t b = out.data[i + 1];

        int16_t avg = (a + b) / 2;
        bool a_larger = (a >= b);

        // Максимально возможное new_diff, чтобы пиксели остались в [0,255]
        int16_t d_max = std::min(
            static_cast<int16_t>(2 * avg),
            static_cast<int16_t>(2 * (255 - avg)));

        // Если даже mask не влезает — пропускаем
        if (d_max < static_cast<int16_t>(mask))
            continue;

        // Читаем PVD_count бит
        uint8_t value = 0;
        for (uint8_t b = 0; b < PVD_count; b++)
        {
            if (bit_index >= total_bits)
                break;
            size_t byte_idx = bit_index / 8;
            uint8_t bit_in_byte = 7 - (bit_index % 8);
            uint8_t bit = (payload[byte_idx] >> bit_in_byte) & 1;
            value |= (bit << (PVD_count - b - 1));
            bit_index++;
        }
        if (bit_index > total_bits)
            break;

        // Полностью заменяем разность на закодированное значение
        int16_t new_diff = static_cast<int16_t>(value);

        int16_t a_new, b_new;
        if (a_larger)
        {
            a_new = avg + new_diff / 2 + (new_diff % 2);
            b_new = avg - new_diff / 2;
        }
        else
        {
            b_new = avg + new_diff / 2 + (new_diff % 2);
            a_new = avg - new_diff / 2;
        }

        out.data[i] = static_cast<uint8_t>(a_new);
        out.data[i + 1] = static_cast<uint8_t>(b_new);
    }

    return out;
}

SecretBytes PVDEncoder::decode(const Image &input)
{
    const size_t total_pixels = input.data.size();
    const uint8_t mask = (1 << PVD_count) - 1;

    std::vector<uint8_t> extracted_bytes;
    extracted_bytes.reserve(1024);

    uint8_t current_byte = 0;
    uint8_t bits_collected = 0;

    uint32_t secret_size = 0;
    bool size_read = false;

    for (size_t i = 0; i + 1 < total_pixels; i += 2)
    {
        if (size_read &&
            extracted_bytes.size() == sizeof(uint32_t) + secret_size)
        {
            break;
        }

        int16_t a = input.data[i];
        int16_t b = input.data[i + 1];

        int16_t avg = (a + b) / 2;

        int16_t d_max = std::min(
            static_cast<int16_t>(2 * avg),
            static_cast<int16_t>(2 * (255 - avg)));

        if (d_max < static_cast<int16_t>(mask))
            continue;

        // Извлекаем закодированное значение — это разность между пикселями
        int16_t diff = static_cast<int16_t>(std::abs(a - b));
        uint8_t value = static_cast<uint8_t>(diff);

        for (int b = PVD_count - 1; b >= 0; b--)
        {
            uint8_t bit = (value >> b) & 1;

            current_byte = (current_byte << 1) | bit;
            bits_collected++;

            if (bits_collected == 8)
            {
                extracted_bytes.push_back(current_byte);
                current_byte = 0;
                bits_collected = 0;

                if (!size_read &&
                    extracted_bytes.size() == sizeof(uint32_t))
                {
                    secret_size =
                        (static_cast<uint32_t>(extracted_bytes[0]) << 0) |
                        (static_cast<uint32_t>(extracted_bytes[1]) << 8) |
                        (static_cast<uint32_t>(extracted_bytes[2]) << 16) |
                        (static_cast<uint32_t>(extracted_bytes[3]) << 24);

                    size_read = true;
                    extracted_bytes.reserve(sizeof(uint32_t) + secret_size);
                }
            }
        }
    }

    if (size_read &&
        extracted_bytes.size() == sizeof(uint32_t) + secret_size)
    {
        SecretBytes result(
            extracted_bytes.begin() + sizeof(uint32_t),
            extracted_bytes.end());

        return result;
    }

    throw std::runtime_error("Failed to decode secret");
}