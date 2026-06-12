#pragma once

#include "../base/CompareAlgorithm.h"

class PSNR : public CompareAlgorithm
{
public:
    double compare(const Image &img1, const Image &img2) override;
};