#pragma once

#include "../../core/Image.h"
#include "../../core/Secret.h"

class StegoAlgorithm
{
public:
    virtual Image encode(const Image &input, const SecretBytes &secret) = 0;
    virtual ~StegoAlgorithm() = default;
};