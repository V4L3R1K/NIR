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
};