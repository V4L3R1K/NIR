#include "Controller.h"

void Controller::perform(const Command &cmd)
{
    switch (cmd.type)
    {
    case CommandType::ENCODE:
        if (cmd.algorithm == "LSB")
        {
            Image img;
            LSBEncoder encoder;
            img.load(cmd.input);
            SecretBytes secret = Secret::load(cmd.secret);

            Image result = encoder.encode(img, secret);

            result.savePNG(cmd.output);
        }
        else
            throw std::runtime_error("unknown algorithm");
        break;
    case CommandType::DECODE:
        break;
    case CommandType::COMPARE:
        break;
    }
}