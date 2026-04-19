#pragma once

#include <string>

#include "Command.h"

class CLIParser
{
public:
    static Command parse(int argc, char *argv[]);
};