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

void Secret::save(const std::string &path, const SecretBytes &secret)
{
    std::ofstream file(path, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("failed to open file for writing: " + path);
    }

    if (!secret.empty())
    {
        if (!file.write(reinterpret_cast<const char *>(secret.data()), secret.size()))
        {
            throw std::runtime_error("failed to write file: " + path);
        }
    }

    if (!file.good())
    {
        throw std::runtime_error("file stream error after writing: " + path);
    }
}