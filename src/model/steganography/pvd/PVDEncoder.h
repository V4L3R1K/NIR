#pragma once

#include "../base/StegoAlgorithm.h"

class PVDEncoder : public StegoAlgorithm
{
public:
    uint8_t PVD_count = 1;

    size_t get_capacity(const Image &input) override;

    Image encode(const Image &input, const SecretBytes &secret) override;
    SecretBytes decode(const Image &input) override;
};