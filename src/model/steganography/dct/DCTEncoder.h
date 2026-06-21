#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <jpeglib.h>

#include "../../core/Image.h"
#include "../../core/Secret.h"

class DCTEncoder
{
public:
    explicit DCTEncoder(int jpeg_quality = 90);

    size_t get_capacity(const Image &input) const;

    void encode(const Image &input,
                const SecretBytes &secret,
                const std::string &output_path);

    SecretBytes decode(const std::string &input_path);

private:
    int quality;

    static std::vector<uint8_t> image_to_rgb(const Image &input);

    static std::vector<uint8_t> extract_bits_from_coefficients(struct jpeg_decompress_struct &dinfo,
                                                               jvirt_barray_ptr *coef_arrays,
                                                               size_t bits_needed);

    static void embed_bits_in_coefficients(struct jpeg_decompress_struct &dinfo,
                                           jvirt_barray_ptr *coef_arrays,
                                           const std::vector<uint8_t> &payload_bits);

    static bool coefficient_usable(int coef);
};