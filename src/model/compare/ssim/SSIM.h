#pragma once

#include "../base/CompareAlgorithm.h"

#include <cmath>
#include <stdexcept>
#include <vector>

class SSIM : public CompareAlgorithm
{
public:
    double compare(const Image &img1, const Image &img2) override;
};