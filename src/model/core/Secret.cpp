#include "Secret.h"

SecretBytes Secret::load(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("failed to open file: " + path);
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0)
    {
        throw std::runtime_error("failed to read file size");
    }

    SecretBytes buffer(size);

    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        throw std::runtime_error("failed to read file");
    }

    return buffer;
}
