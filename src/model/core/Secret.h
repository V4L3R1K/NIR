#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>

typedef std::vector<uint8_t> SecretBytes;

class Secret
{
public:
    static SecretBytes load(const std::string &path);
    static void save(const std::string &path, const SecretBytes &secret);

    static SecretBytes prepare(const SecretBytes &original, size_t capacity);
    static SecretBytes recover(const SecretBytes &payload);

private:
    static constexpr size_t RS_SYMBOL_SIZE = 8;          // 8-bit symbols
    static constexpr uint16_t RS_PRIMITIVE_POLY = 0x11d; // x^8 + x^4 + x^3 + x^2 + 1
    static constexpr uint8_t RS_FIRST_ROOT = 1;
    static constexpr uint8_t RS_ROOT_GAP = 1;
    static constexpr size_t RS_NUM_ROOTS = 32;    // can correct up to 16 bytes per block
    static constexpr size_t RS_BLOCK_DATA = 223;  // 255 - 32
    static constexpr size_t RS_BLOCK_TOTAL = 255; // RS_BLOCK_DATA + RS_NUM_ROOTS
};