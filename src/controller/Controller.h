#pragma once

#include <stdexcept>

#include "../view/cli/Command.h"
#include "../model/steganography/lsb/LSBEncoder.h"
#include "../model/core/Image.h"
#include "../model/core/Secret.h"

class Controller
{
public:
    static void perform(const Command &cmd);
};