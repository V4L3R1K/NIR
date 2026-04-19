#include "Controller.h"

void Controller::perform(const Command &cmd)
{
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
        else
            throw std::runtime_error("unknown algorithm");
        break;
    case CommandType::COMPARE:
        break;
    default:
        throw std::runtime_error("unknown command");
        break;
    }
}