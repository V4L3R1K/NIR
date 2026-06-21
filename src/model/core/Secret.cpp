#include "Secret.h"

#include <cstring>
#include <algorithm>

extern "C"
{
#include "correct.h"
}

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

SecretBytes Secret::prepare(const SecretBytes &original, size_t capacity)
{
    // Minimum capacity: at least 4 bytes for header
    if (capacity < 4)
    {
        throw std::runtime_error("Capacity too small for header");
    }

    // If capacity is too small for even one RS block, encode without ECC
    if (capacity < RS_BLOCK_TOTAL)
    {
        SecretBytes result(capacity, 0);
        uint32_t orig_size = static_cast<uint32_t>(original.size());
        result[0] = (orig_size >> 0) & 0xFF;
        result[1] = (orig_size >> 8) & 0xFF;
        result[2] = (orig_size >> 16) & 0xFF;
        result[3] = (orig_size >> 24) & 0xFF;
        size_t copy_len = std::min(original.size(), capacity - 4);
        std::copy(original.begin(), original.begin() + copy_len, result.begin() + 4);
        return result;
    }

    const size_t num_blocks = capacity / RS_BLOCK_TOTAL;
    const size_t total_data_slots = num_blocks * RS_BLOCK_DATA; // 223 * num_blocks

    // 4 bytes for original size header, rest for data
    const size_t max_data = total_data_slots - 4;
    if (original.size() > max_data)
    {
        throw std::runtime_error("Secret is too large for this capacity");
    }

    // Build data block: [4-byte size][original data][zero padding to total_data_slots]
    SecretBytes data(total_data_slots, 0);
    uint32_t orig_size = static_cast<uint32_t>(original.size());
    data[0] = (orig_size >> 0) & 0xFF;
    data[1] = (orig_size >> 8) & 0xFF;
    data[2] = (orig_size >> 16) & 0xFF;
    data[3] = (orig_size >> 24) & 0xFF;
    std::copy(original.begin(), original.end(), data.begin() + 4);

    // Create Reed-Solomon encoder
    correct_reed_solomon *rs = correct_reed_solomon_create(
        RS_PRIMITIVE_POLY, RS_FIRST_ROOT, RS_ROOT_GAP, RS_NUM_ROOTS);
    if (!rs)
    {
        throw std::runtime_error("Failed to create Reed-Solomon encoder");
    }

    // Encode each block
    SecretBytes result(capacity, 0);
    for (size_t b = 0; b < num_blocks; ++b)
    {
        const uint8_t *msg = data.data() + b * RS_BLOCK_DATA;
        uint8_t *encoded = result.data() + b * RS_BLOCK_TOTAL;

        ssize_t ret = correct_reed_solomon_encode(rs, msg, RS_BLOCK_DATA, encoded);
        if (ret < 0)
        {
            correct_reed_solomon_destroy(rs);
            throw std::runtime_error("Reed-Solomon encoding failed");
        }
    }

    correct_reed_solomon_destroy(rs);
    return result;
}

SecretBytes Secret::recover(const SecretBytes &payload)
{
    // If payload is smaller than one RS block, decode without ECC
    if (payload.size() < RS_BLOCK_TOTAL)
    {
        if (payload.size() < 4)
        {
            throw std::runtime_error("Payload too small for header");
        }
        uint32_t orig_size =
            (static_cast<uint32_t>(payload[0]) << 0) |
            (static_cast<uint32_t>(payload[1]) << 8) |
            (static_cast<uint32_t>(payload[2]) << 16) |
            (static_cast<uint32_t>(payload[3]) << 24);
        if (orig_size > payload.size() - 4)
        {
            throw std::runtime_error("Corrupted secret: invalid size in header");
        }
        SecretBytes result(payload.begin() + 4, payload.begin() + 4 + orig_size);
        return result;
    }

    const size_t num_blocks = payload.size() / RS_BLOCK_TOTAL;
    const size_t total_data_slots = num_blocks * RS_BLOCK_DATA;

    // Create Reed-Solomon decoder
    correct_reed_solomon *rs = correct_reed_solomon_create(
        RS_PRIMITIVE_POLY, RS_FIRST_ROOT, RS_ROOT_GAP, RS_NUM_ROOTS);
    if (!rs)
    {
        throw std::runtime_error("Failed to create Reed-Solomon decoder");
    }

    // Decode each block
    SecretBytes decoded_data(total_data_slots, 0);
    for (size_t b = 0; b < num_blocks; ++b)
    {
        const uint8_t *encoded = payload.data() + b * RS_BLOCK_TOTAL;
        uint8_t *msg = decoded_data.data() + b * RS_BLOCK_DATA;

        ssize_t ret = correct_reed_solomon_decode(rs, encoded, RS_BLOCK_TOTAL, msg);
        if (ret < 0)
        {
            correct_reed_solomon_destroy(rs);
            throw std::runtime_error("Reed-Solomon decoding failed: too many errors");
        }
    }

    correct_reed_solomon_destroy(rs);

    // Read original size from header
    uint32_t orig_size =
        (static_cast<uint32_t>(decoded_data[0]) << 0) |
        (static_cast<uint32_t>(decoded_data[1]) << 8) |
        (static_cast<uint32_t>(decoded_data[2]) << 16) |
        (static_cast<uint32_t>(decoded_data[3]) << 24);

    if (orig_size > decoded_data.size() - 4)
    {
        throw std::runtime_error("Corrupted secret: invalid size in header");
    }

    // Return only the original data (without header)
    SecretBytes result(decoded_data.begin() + 4, decoded_data.begin() + 4 + orig_size);
    return result;
}