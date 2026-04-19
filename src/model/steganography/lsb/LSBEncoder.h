#pragma once

#include "../base/StegoAlgorithm.h"

class LSBEncoder : public StegoAlgorithm
{
public:
    uint8_t LSB_count = 2;

    size_t get_capacity(const Image &input) override;

    Image encode(const Image &input, const SecretBytes &secret) override;
    SecretBytes decode(const Image &input) override;
};