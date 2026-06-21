#include "DCTEncoder.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <iostream>

#include <jpeglib.h>

namespace
{
    constexpr int DCT_COEF_COUNT = 64;
}

DCTEncoder::DCTEncoder(int jpeg_quality)
    : quality(jpeg_quality)
{
    if (quality < 1 || quality > 100)
    {
        throw std::runtime_error("JPEG quality must be in range [1, 100]");
    }
}

std::vector<uint8_t> DCTEncoder::image_to_rgb(const Image &input)
{
    if (input.width <= 0 || input.height <= 0)
    {
        throw std::runtime_error("Invalid image dimensions");
    }

    if (input.data.empty())
    {
        throw std::runtime_error("Image data is empty");
    }

    std::vector<uint8_t> rgb;
    rgb.reserve(static_cast<size_t>(input.width) * static_cast<size_t>(input.height) * 3);

    if (input.channels == 3)
    {
        rgb = input.data;
        return rgb;
    }

    if (input.channels == 4)
    {
        for (size_t i = 0; i + 3 < input.data.size(); i += 4)
        {
            rgb.push_back(input.data[i + 0]);
            rgb.push_back(input.data[i + 1]);
            rgb.push_back(input.data[i + 2]);
        }
        return rgb;
    }

    if (input.channels == 1)
    {
        for (uint8_t v : input.data)
        {
            rgb.push_back(v);
            rgb.push_back(v);
            rgb.push_back(v);
        }
        return rgb;
    }

    throw std::runtime_error("Unsupported channel count (expected 1, 3, or 4)");
}

bool DCTEncoder::coefficient_usable(int coef)
{
    return std::abs(coef) > 1;
}

size_t DCTEncoder::get_capacity(const Image &input) const
{
    std::vector<uint8_t> rgb = image_to_rgb(input);

    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    std::memset(&cinfo, 0, sizeof(cinfo));

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char *jpeg_buffer = nullptr;
    unsigned long jpeg_size = 0;
    jpeg_mem_dest(&cinfo, &jpeg_buffer, &jpeg_size);

    cinfo.image_width = input.width;
    cinfo.image_height = input.height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    const int row_stride = input.width * 3;
    while (cinfo.next_scanline < cinfo.image_height)
    {
        JSAMPROW row = &rgb[static_cast<size_t>(cinfo.next_scanline) * static_cast<size_t>(row_stride)];
        jpeg_write_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_compress(&cinfo);

    jpeg_decompress_struct dinfo;
    jpeg_error_mgr derr;
    std::memset(&dinfo, 0, sizeof(dinfo));

    dinfo.err = jpeg_std_error(&derr);
    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo, jpeg_buffer, jpeg_size);

    jpeg_read_header(&dinfo, TRUE);
    jvirt_barray_ptr *coef_arrays = jpeg_read_coefficients(&dinfo);

    size_t capacity_bits = 0;

    for (int comp = 0; comp < dinfo.num_components; ++comp)
    {
        const jpeg_component_info &ci = dinfo.comp_info[comp];

        for (JDIMENSION row = 0; row < ci.height_in_blocks; ++row)
        {
            JBLOCKARRAY block_rows = (*dinfo.mem->access_virt_barray)(
                reinterpret_cast<j_common_ptr>(&dinfo),
                coef_arrays[comp],
                row,
                static_cast<JDIMENSION>(1),
                FALSE);

            for (JDIMENSION col = 0; col < ci.width_in_blocks; ++col)
            {
                JCOEFPTR block = block_rows[0][col];

                for (int k = 1; k < DCT_COEF_COUNT; ++k)
                {
                    if (coefficient_usable(block[k]))
                    {
                        ++capacity_bits;
                    }
                }
            }
        }
    }

    jpeg_destroy_decompress(&dinfo);
    jpeg_destroy_compress(&cinfo);
    std::free(jpeg_buffer);

    return capacity_bits / 8;
}

void DCTEncoder::embed_bits_in_coefficients(jpeg_decompress_struct &dinfo,
                                            jvirt_barray_ptr *coef_arrays,
                                            const std::vector<uint8_t> &payload_bits)
{
    size_t bit_index = 0;

    for (int comp = 0; comp < dinfo.num_components && bit_index < payload_bits.size(); ++comp)
    {
        const jpeg_component_info &ci = dinfo.comp_info[comp];

        for (JDIMENSION row = 0; row < ci.height_in_blocks && bit_index < payload_bits.size(); ++row)
        {
            JBLOCKARRAY block_rows = (*dinfo.mem->access_virt_barray)(
                reinterpret_cast<j_common_ptr>(&dinfo),
                coef_arrays[comp],
                row,
                static_cast<JDIMENSION>(1),
                TRUE);

            for (JDIMENSION col = 0; col < ci.width_in_blocks && bit_index < payload_bits.size(); ++col)
            {
                JCOEFPTR block = block_rows[0][col];

                for (int k = 1; k < DCT_COEF_COUNT && bit_index < payload_bits.size(); ++k)
                {
                    int coef = static_cast<int>(block[k]);

                    if (!coefficient_usable(coef))
                    {
                        continue;
                    }

                    const int desired_bit = payload_bits[bit_index] & 1U;
                    const int current_bit = std::abs(coef) & 1;

                    if (current_bit != desired_bit)
                    {
                        if (coef > 0)
                            ++coef;
                        else
                            --coef;

                        block[k] = static_cast<JCOEF>(coef);
                    }

                    ++bit_index;
                }
            }
        }
    }

    if (bit_index < payload_bits.size())
    {
        throw std::runtime_error("Not enough DCT capacity to embed secret");
    }
}

std::vector<uint8_t> DCTEncoder::extract_bits_from_coefficients(jpeg_decompress_struct &dinfo,
                                                                jvirt_barray_ptr *coef_arrays,
                                                                size_t bits_needed)
{
    std::vector<uint8_t> bits;
    bits.reserve(bits_needed);

    for (int comp = 0; comp < dinfo.num_components && bits.size() < bits_needed; ++comp)
    {
        const jpeg_component_info &ci = dinfo.comp_info[comp];

        for (JDIMENSION row = 0; row < ci.height_in_blocks && bits.size() < bits_needed; ++row)
        {
            JBLOCKARRAY block_rows = (*dinfo.mem->access_virt_barray)(
                reinterpret_cast<j_common_ptr>(&dinfo),
                coef_arrays[comp],
                row,
                static_cast<JDIMENSION>(1),
                FALSE);

            for (JDIMENSION col = 0; col < ci.width_in_blocks && bits.size() < bits_needed; ++col)
            {
                JCOEFPTR block = block_rows[0][col];

                for (int k = 1; k < DCT_COEF_COUNT && bits.size() < bits_needed; ++k)
                {
                    int coef = static_cast<int>(block[k]);

                    if (!coefficient_usable(coef))
                    {
                        continue;
                    }

                    bits.push_back(static_cast<uint8_t>(std::abs(coef) & 1));
                }
            }
        }
    }

    if (bits.size() < bits_needed)
    {
        throw std::runtime_error("Not enough DCT data to extract payload");
    }

    return bits;
}

void DCTEncoder::encode(const Image &input,
                        const SecretBytes &secret,
                        const std::string &output_path)
{
    std::vector<uint8_t> rgb = image_to_rgb(input);

    // Convert secret bytes to bit vector
    std::vector<uint8_t> payload_bits;
    payload_bits.reserve(secret.size() * 8);
    for (uint8_t byte : secret)
    {
        for (size_t i = 0; i < 8; ++i)
        {
            payload_bits.push_back(static_cast<uint8_t>((byte >> i) & 1U));
        }
    }

    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    std::memset(&cinfo, 0, sizeof(cinfo));

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char *jpeg_buffer = nullptr;
    unsigned long jpeg_size = 0;
    jpeg_mem_dest(&cinfo, &jpeg_buffer, &jpeg_size);

    cinfo.image_width = input.width;
    cinfo.image_height = input.height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    const int row_stride = input.width * 3;
    while (cinfo.next_scanline < cinfo.image_height)
    {
        JSAMPROW row = &rgb[static_cast<size_t>(cinfo.next_scanline) * static_cast<size_t>(row_stride)];
        jpeg_write_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_compress(&cinfo);

    jpeg_decompress_struct dinfo;
    jpeg_error_mgr derr;
    std::memset(&dinfo, 0, sizeof(dinfo));

    dinfo.err = jpeg_std_error(&derr);
    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo, jpeg_buffer, jpeg_size);

    jpeg_read_header(&dinfo, TRUE);
    jvirt_barray_ptr *coef_arrays = jpeg_read_coefficients(&dinfo);

    size_t capacity_bits = 0;

    for (int comp = 0; comp < dinfo.num_components; ++comp)
    {
        const jpeg_component_info &ci = dinfo.comp_info[comp];

        for (JDIMENSION row = 0; row < ci.height_in_blocks; ++row)
        {
            JBLOCKARRAY block_rows = (*dinfo.mem->access_virt_barray)(
                reinterpret_cast<j_common_ptr>(&dinfo),
                coef_arrays[comp],
                row,
                static_cast<JDIMENSION>(1),
                FALSE);

            for (JDIMENSION col = 0; col < ci.width_in_blocks; ++col)
            {
                JCOEFPTR block = block_rows[0][col];

                for (int k = 1; k < DCT_COEF_COUNT; ++k)
                {
                    if (coefficient_usable(block[k]))
                    {
                        ++capacity_bits;
                    }
                }
            }
        }
    }

    // Verify secret matches get_capacity
    size_t expected_capacity = get_capacity(input);
    if (secret.size() != expected_capacity)
    {
        jpeg_destroy_decompress(&dinfo);
        jpeg_destroy_compress(&cinfo);
        std::free(jpeg_buffer);
        throw std::runtime_error("Secret size does not match capacity (call Secret::prepare first)");
    }

    // Use only as many bits as we have usable coefficients
    if (payload_bits.size() > capacity_bits)
    {
        payload_bits.resize(capacity_bits);
    }

    embed_bits_in_coefficients(dinfo, coef_arrays, payload_bits);

    jpeg_compress_struct outinfo;
    jpeg_error_mgr outjerr;
    std::memset(&outinfo, 0, sizeof(outinfo));

    outinfo.err = jpeg_std_error(&outjerr);
    jpeg_create_compress(&outinfo);

    FILE *outfile = std::fopen(output_path.c_str(), "wb");
    if (!outfile)
    {
        jpeg_destroy_decompress(&dinfo);
        jpeg_destroy_compress(&cinfo);
        jpeg_destroy_compress(&outinfo);
        std::free(jpeg_buffer);
        throw std::runtime_error("Failed to open output file");
    }

    jpeg_stdio_dest(&outinfo, outfile);
    jpeg_copy_critical_parameters(&dinfo, &outinfo);
    jpeg_write_coefficients(&outinfo, coef_arrays);

    jpeg_finish_compress(&outinfo);
    std::fclose(outfile);

    jpeg_destroy_decompress(&dinfo);
    jpeg_destroy_compress(&outinfo);
    jpeg_destroy_compress(&cinfo);
    std::free(jpeg_buffer);
}

SecretBytes DCTEncoder::decode(const std::string &input_path)
{
    FILE *infile = std::fopen(input_path.c_str(), "rb");
    if (!infile)
    {
        throw std::runtime_error("Failed to open JPEG file");
    }

    jpeg_decompress_struct dinfo;
    jpeg_error_mgr jerr;
    std::memset(&dinfo, 0, sizeof(dinfo));

    dinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&dinfo);
    jpeg_stdio_src(&dinfo, infile);

    jpeg_read_header(&dinfo, TRUE);
    jvirt_barray_ptr *coef_arrays = jpeg_read_coefficients(&dinfo);

    // Calculate capacity bits
    size_t capacity_bits = 0;
    for (int comp = 0; comp < dinfo.num_components; ++comp)
    {
        const jpeg_component_info &ci = dinfo.comp_info[comp];

        for (JDIMENSION row = 0; row < ci.height_in_blocks; ++row)
        {
            JBLOCKARRAY block_rows = (*dinfo.mem->access_virt_barray)(
                reinterpret_cast<j_common_ptr>(&dinfo),
                coef_arrays[comp],
                row,
                static_cast<JDIMENSION>(1),
                FALSE);

            for (JDIMENSION col = 0; col < ci.width_in_blocks; ++col)
            {
                JCOEFPTR block = block_rows[0][col];

                for (int k = 1; k < DCT_COEF_COUNT; ++k)
                {
                    if (coefficient_usable(block[k]))
                    {
                        ++capacity_bits;
                    }
                }
            }
        }
    }

    size_t capacity_bytes = capacity_bits / 8;
    size_t total_bits_needed = capacity_bytes * 8;

    std::vector<uint8_t> all_bits = extract_bits_from_coefficients(dinfo, coef_arrays, total_bits_needed);

    jpeg_destroy_decompress(&dinfo);
    std::fclose(infile);

    // Convert bits to bytes
    SecretBytes result(capacity_bytes, 0);
    for (size_t i = 0; i < total_bits_needed; ++i)
    {
        size_t byte_idx = i / 8;
        uint8_t bit_in_byte = i % 8;
        result[byte_idx] |= (all_bits[i] & 1U) << bit_in_byte;
    }

    return result;
}