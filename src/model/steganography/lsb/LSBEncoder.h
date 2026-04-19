#pragma once
#include "../base/StegoAlgorithm.h"

class LSBEncoder : public StegoAlgorithm {
public:
    Image encode(const Image& input) override;
};