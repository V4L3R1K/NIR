#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class CommandType
{
    ENCODE,
    DECODE,
    COMPARE,
    UNKNOWN
};

struct Command
{
    CommandType type = CommandType::UNKNOWN; // -E or -D or -C
    std::string algorithm = "";              // -A
    std::string input = "";                  // -i
    std::string input_compare = "";          // -i
    std::string secret = "";                 // -s
    std::string output = "";                 // -o
    uint8_t lsb_count = 2;                   // --lsb-count
};