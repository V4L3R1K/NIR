#include "LSBEncoder.h"

Image LSBEncoder::encode(const Image& input) {
    Image out = input;

    // пиксель (0,0) → чёрный
    int idx = 0; // первый пиксель

    out.data[idx + 0] = 0; // R
    out.data[idx + 1] = 0; // G
    out.data[idx + 2] = 0; // B

    return out;
}