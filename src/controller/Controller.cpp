#include "Controller.h"

// Helper: compute max data size for the given raw capacity
// with RS(255,223) encoding and \0 terminator (1 byte overhead)
static size_t max_data_size(size_t raw_capacity)
{
    const size_t RS_BLOCK_TOTAL = 255;
    const size_t RS_BLOCK_DATA = 223;
    if (raw_capacity < RS_BLOCK_TOTAL)
    {
        // Need 1 byte for \0 terminator
        return (raw_capacity > 0) ? (raw_capacity - 1) : 0;
    }
    size_t num_blocks = raw_capacity / RS_BLOCK_TOTAL;
    // Reserve 1 byte for \0 terminator inside each total_data_slots
    if (num_blocks * RS_BLOCK_DATA == 0)
        return 0;
    return num_blocks * RS_BLOCK_DATA - 1;
}

void Controller::perform(const Command &cmd)
{
    if (cmd.capacity)
    {
        Image img;
        img.load(cmd.input);

        if (cmd.algorithm == "LSB")
        {
            LSBEncoder encoder;
            encoder.LSB_count = cmd.lsb_count;
            size_t cap = encoder.get_capacity(img);
            std::cout << max_data_size(cap) << std::endl;
        }
        else if (cmd.algorithm == "DCT")
        {
            DCTEncoder encoder(cmd.jpeg_quality);
            size_t cap = encoder.get_capacity(img);
            std::cout << max_data_size(cap) << std::endl;
        }
        else if (cmd.algorithm == "DWT")
        {
            DWTEncoder encoder;
            size_t cap = encoder.get_capacity(img);
            std::cout << max_data_size(cap) << std::endl;
        }
        else if (cmd.algorithm == "PVD")
        {
            PVDEncoder encoder;
            encoder.PVD_count = cmd.pvd_count;
            size_t cap = encoder.get_capacity(img);
            std::cout << max_data_size(cap) << std::endl;
        }
        else
            throw std::runtime_error("unknown algorithm for capacity");

        return;
    }

    switch (cmd.type)
    {
    case CommandType::ENCODE:
        if (cmd.algorithm == "LSB")
        {
            LSBEncoder encoder;
            encoder.LSB_count = cmd.lsb_count;

            Image img;
            img.load(cmd.input);

            SecretBytes original = Secret::load(cmd.secret);
            size_t capacity = encoder.get_capacity(img);
            SecretBytes prepared = Secret::prepare(original, capacity);

            Image result = encoder.encode(img, prepared);
            result.savePNG(cmd.output);
        }
        else if (cmd.algorithm == "DCT")
        {
            DCTEncoder encoder(cmd.jpeg_quality);

            Image img;
            img.load(cmd.input);

            SecretBytes original = Secret::load(cmd.secret);
            size_t capacity = encoder.get_capacity(img);
            SecretBytes prepared = Secret::prepare(original, capacity);

            encoder.encode(img, prepared, cmd.output);
        }
        else if (cmd.algorithm == "DWT")
        {
            DWTEncoder encoder;

            Image img;
            img.load(cmd.input);

            SecretBytes original = Secret::load(cmd.secret);
            size_t capacity = encoder.get_capacity(img);
            SecretBytes prepared = Secret::prepare(original, capacity);

            encoder.encode(img, prepared, cmd.output);
        }
        else if (cmd.algorithm == "PVD")
        {
            PVDEncoder encoder;
            encoder.PVD_count = cmd.pvd_count;

            Image img;
            img.load(cmd.input);

            SecretBytes original = Secret::load(cmd.secret);
            size_t capacity = encoder.get_capacity(img);
            SecretBytes prepared = Secret::prepare(original, capacity);

            Image result = encoder.encode(img, prepared);
            result.savePNG(cmd.output);
        }
        else
            throw std::runtime_error("unknown algorithm");
        break;
    case CommandType::DECODE:
        if (cmd.algorithm == "LSB")
        {
            LSBEncoder encoder;
            encoder.LSB_count = cmd.lsb_count;

            Image img;
            img.load(cmd.input);

            SecretBytes payload = encoder.decode(img);
            SecretBytes original = Secret::recover(payload);

            Secret::save(cmd.output, original);
        }
        else if (cmd.algorithm == "DCT")
        {
            DCTEncoder encoder(cmd.jpeg_quality);

            SecretBytes payload = encoder.decode(cmd.input);
            SecretBytes original = Secret::recover(payload);

            Secret::save(cmd.output, original);
        }
        else if (cmd.algorithm == "DWT")
        {
            DWTEncoder encoder;

            SecretBytes payload = encoder.decode(cmd.input);
            SecretBytes original = Secret::recover(payload);

            Secret::save(cmd.output, original);
        }
        else if (cmd.algorithm == "PVD")
        {
            PVDEncoder encoder;
            encoder.PVD_count = cmd.pvd_count;

            Image img;
            img.load(cmd.input);

            SecretBytes payload = encoder.decode(img);
            SecretBytes original = Secret::recover(payload);

            Secret::save(cmd.output, original);
        }
        else
            throw std::runtime_error("unknown algorithm");
        break;
    case CommandType::COMPARE:
    {
        CompareAlgorithm *algo = nullptr;

        if (cmd.algorithm == "PSNR")
            algo = new PSNR();
        else if (cmd.algorithm == "SSIM")
            algo = new SSIM();
        else
            throw std::runtime_error("unknown compare algorithm");

        Image img1, img2;
        img1.load(cmd.input);
        img2.load(cmd.input_compare);

        double result = algo->compare(img1, img2);

        if (cmd.algorithm == "PSNR")
            std::cout << result << std::endl;
        else if (cmd.algorithm == "SSIM")
            std::cout << result << std::endl;

        delete algo;
        break;
    }
    default:
        throw std::runtime_error("unknown command");
        break;
    }
}