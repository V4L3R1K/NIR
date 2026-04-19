#pragma once
#include "../../core/Image.h"

class StegoAlgorithm {
public:
    virtual Image encode(const Image& input) = 0;
    virtual ~StegoAlgorithm() = default;
};