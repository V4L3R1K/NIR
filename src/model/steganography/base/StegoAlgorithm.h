#pragma once

#include "../../core/Image.h"
#include "../../core/Secret.h"

class StegoAlgorithm
{
public:
    virtual size_t get_capacity(const Image &input) = 0;
    virtual Image encode(const Image &input, const SecretBytes &secret) = 0;
    virtual SecretBytes decode(const Image &input) = 0;
    virtual ~StegoAlgorithm() = default;
};