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
    // If capacity is zero, nothing can be embedded
    if (capacity == 0)
    {
        if (!original.empty())
        {
            throw std::runtime_error("Secret is too large for this capacity");
        }
        return {};
    }

    // Need at least 1 byte for the \0 terminator
    if (original.size() + 1 > capacity)
    {
        throw std::runtime_error("Secret is too large for this capacity");
    }

    // If capacity is too small for even one RS block, just pad with zeros
    if (capacity < RS_BLOCK_TOTAL)
    {
        SecretBytes result(capacity, 0);
        std::copy(original.begin(), original.end(), result.begin());
        // result[original.size()] is already 0 (the \0 terminator)
        return result;
    }

    const size_t num_blocks = capacity / RS_BLOCK_TOTAL;
    const size_t total_data_slots = num_blocks * RS_BLOCK_DATA; // 223 * num_blocks

    if (original.size() + 1 > total_data_slots)
    {
        throw std::runtime_error("Secret is too large for this capacity");
    }

    // Build data block: [original data][\0 terminator][zero padding to total_data_slots]
    SecretBytes data(total_data_slots, 0);
    std::copy(original.begin(), original.end(), data.begin());
    // data[original.size()] is already 0 (the \0 terminator)

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
    if (payload.empty())
    {
        return {};
    }

    SecretBytes decoded_data;

    if (payload.size() < RS_BLOCK_TOTAL)
    {
        // Without RS: payload itself is the data
        decoded_data = payload;
    }
    else
    {
        // With RS: decode each block
        const size_t num_blocks = payload.size() / RS_BLOCK_TOTAL;
        const size_t total_data_slots = num_blocks * RS_BLOCK_DATA;

        correct_reed_solomon *rs = correct_reed_solomon_create(
            RS_PRIMITIVE_POLY, RS_FIRST_ROOT, RS_ROOT_GAP, RS_NUM_ROOTS);
        if (!rs)
        {
            throw std::runtime_error("Failed to create Reed-Solomon decoder");
        }

        decoded_data.assign(total_data_slots, 0);
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
    }

    // Trim trailing zeros: the format is [original data][zero padding to capacity].
    // Find the last non-zero byte; everything after it is padding.
    // For messages ending with \0, that \0 is preserved as the last non-zero?
    // No - the \0 is zero, so it gets trimmed. But the user said the message
    // *itself* ends with \0, so prepare() stores message including that \0.
    // After preparation: [message (with trailing \0)][zeros].
    // RS decode reconstructs this. Trimming trailing zeros removes the padding
    // and the extra \0 that prepare() added after the original message.
    // But actually prepare() does NOT add extra \0 - it copies original as-is
    // and leaves the rest as zeros. The original's own bytes include any \0
    // at the end. So decoding gives [original data][zeros].
    // Trimming trailing zeros leaves [original data] unchanged.
    size_t data_len = decoded_data.size();
    while (data_len > 0 && decoded_data[data_len - 1] == 0)
    {
        --data_len;
    }

    SecretBytes result(decoded_data.begin(), decoded_data.begin() + data_len);
    return result;
}