#include <iostream>
#include "CLIParser.h"

static std::string stripQuotes(const std::string &s)
{
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
         (s.front() == '\'' && s.back() == '\'')))
    {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

static void printHelp()
{
    std::cout << "Usage: NIR [options]\n"
                 "\n"
                 "Options:\n"
                 "  -h, --help          Show this help message and exit\n"
                 "\n"
                 "Mode (one required):\n"
                 "  -E                  Encode mode — encode secret into container\n"
                 "  -D                  Decode mode — decode secret from container\n"
                 "  -C                  Compare mode — compare two images\n"
                 "\n"
                 "Algorithm:\n"
                 "  -A <name>           Steganography algorithm: LSB, PVD, DCT, DWT\n"
                 "                      Compare algorithm:       PSNR, SSIM\n"
                 "\n"
                 "Input / Output:\n"
                 "  -i <path>           Input container image path (for -E, -D)\n"
                 "                      Second -i is used as second image for -C\n"
                 "  -s <path>           Secret file path (for -E)\n"
                 "  -o <path>           Output file path (stego image / extracted secret)\n"
                 "\n"
                 "Tuning (optional):\n"
                 "  --lsb-count <N>     Number of LSB bits to use (default: 2)\n"
                 "  --pvd-count <N>     Number of PVD bits to use (default: 1)\n"
                 "  --jpeg-quality <N>  JPEG quality for output for DCT (default: 90)\n"
                 "  --capacity          Print container capacity in bytes and exit\n"
                 "\n"
                 "Examples:\n"
                 "  # Encode secret into container using LSB\n"
                 "  NIR -E -A LSB -i container.png -s secret.txt -o stego.png\n"
                 "\n"
                 "  # Decode secret from stego image\n"
                 "  NIR -D -A LSB -i stego.png -o extracted.txt\n"
                 "\n"
                 "  # Compare two images\n"
                 "  NIR -C -A PSNR -i original.png -i modified.png\n"
                 "\n"
                 "  # Show container capacity\n"
                 "  NIR -E -A LSB -i container.png --capacity\n";
}

Command CLIParser::parse(int argc, char *argv[])
{
    Command cmd;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        // help
        if (arg == "-h" || arg == "--help")
        {
            printHelp();
            exit(0);
        }

        // mode
        else if (arg == "-E")
        {
            cmd.type = CommandType::ENCODE;
        }
        else if (arg == "-D")
        {
            cmd.type = CommandType::DECODE;
        }
        else if (arg == "-C")
        {
            cmd.type = CommandType::COMPARE;
        }

        // algorithm
        else if (arg == "-A" && i + 1 < argc)
        {
            cmd.algorithm = stripQuotes(argv[++i]);
        }

        // input
        else if (arg == "-i" && i + 1 < argc)
        {
            std::string value = stripQuotes(argv[++i]);
            if (cmd.input.empty())
                cmd.input = value;
            else
                cmd.input_compare = value;
        }

        // secret
        else if (arg == "-s" && i + 1 < argc)
        {
            cmd.secret = stripQuotes(argv[++i]);
        }

        // output
        else if (arg == "-o" && i + 1 < argc)
        {
            cmd.output = stripQuotes(argv[++i]);
        }

        // сколько битов are least significant
        else if (arg == "--lsb-count" && i + 1 < argc)
        {
            cmd.lsb_count = atoi(argv[++i]);
        }

        // количество бит для PVD
        else if (arg == "--pvd-count" && i + 1 < argc)
        {
            cmd.pvd_count = atoi(argv[++i]);
        }

        // качество джипега
        else if (arg == "--jpeg-quality" && i + 1 < argc)
        {
            cmd.jpeg_quality = atoi(argv[++i]);
        }

        // вывод вместимости контейнера
        else if (arg == "--capacity")
        {
            cmd.capacity = true;
        }
    }

    return cmd;
}