#pragma once

#include <stdexcept>
#include <iostream>

#include "../view/cli/Command.h"
#include "../model/steganography/lsb/LSBEncoder.h"
#include "../model/core/Image.h"
#include "../model/core/Secret.h"
#include "../model/compare/psnr/PSNR.h"
#include "../model/compare/ssim/SSIM.h"

class Controller
{
public:
    static void perform(const Command &cmd);
};