#include "Controller.h"

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
            // 4 байта — заголовок с размером секрета
            std::cout << (cap >= 4 ? cap - 4 : 0) << std::endl;
        }
        else if (cmd.algorithm == "DCT")
        {
            DCTEncoder encoder(cmd.jpeg_quality);
            size_t cap = encoder.get_capacity(img);
            // 32 бита — заголовок с размером секрета
            std::cout << (cap >= 32 ? (cap - 32) / 8 : 0) << std::endl;
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

            SecretBytes secret = Secret::load(cmd.secret);

            Image result = encoder.encode(img, secret);

            result.savePNG(cmd.output);
        }
        else if (cmd.algorithm == "DCT")
        {
            DCTEncoder encoder(cmd.jpeg_quality);

            Image img;
            img.load(cmd.input);

            SecretBytes secret = Secret::load(cmd.secret);

            encoder.encode(img, secret, cmd.output);
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

            SecretBytes result = encoder.decode(img);

            Secret::save(cmd.output, result);
        }
        else if (cmd.algorithm == "DCT")
        {
            DCTEncoder encoder(cmd.jpeg_quality);

            SecretBytes result = encoder.decode(cmd.input);

            Secret::save(cmd.output, result);
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
