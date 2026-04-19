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

Command CLIParser::parse(int argc, char *argv[])
{
    Command cmd;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        // mode
        if (arg == "-E")
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
            cmd.input = stripQuotes(argv[++i]);
        }

        // input 2
        else if (arg == "-i" && i + 1 < argc)
        {
            cmd.input_compare = stripQuotes(argv[++i]);
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

        else if (arg == "--lsb-count" && i + 1 < argc)
        {
            cmd.lsb_count = atoi(argv[++i]);
        }
    }

    return cmd;
}