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
    return bits / 8;
}

Image PVDEncoder::encode(const Image &input, const SecretBytes &secret)
{
    size_t capacity = get_capacity(input);

    if (secret.size() != capacity)
        throw std::runtime_error("Secret size does not match capacity (call Secret::prepare first)");

    Image out = input;

    const size_t total_bits = secret.size() * 8;
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
            uint8_t bit = (secret[byte_idx] >> bit_in_byte) & 1;
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
    size_t capacity = get_capacity(input);
    const size_t total_bits = capacity * 8;
    const size_t total_pixels = input.data.size();
    const uint8_t mask = (1 << PVD_count) - 1;

    SecretBytes result(capacity, 0);
    size_t bit_index = 0;

    for (size_t i = 0; i + 1 < total_pixels && bit_index < total_bits; i += 2)
    {
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
            if (bit_index >= total_bits)
                break;

            uint8_t bit = (value >> b) & 1;
            size_t byte_index = bit_index / 8;
            uint8_t bit_in_byte = 7 - (bit_index % 8);

            result[byte_index] |= (bit << bit_in_byte);
            bit_index++;
        }
    }

    return result;
}