#include <iostream>

#include "controller/Controller.h"
#include "view/cli/CLIParser.h"

int main(int argc, char *argv[])
{
    try
    {
        Command cmd = CLIParser::parse(argc, argv);
        Controller::perform(cmd);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}