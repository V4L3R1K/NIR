#pragma once

#include "../../core/Image.h"

class CompareAlgorithm
{
public:
    virtual double compare(const Image &img1, const Image &img2) = 0;
    virtual ~CompareAlgorithm() = default;
};
