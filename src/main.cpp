#include <iostream>
#include <string>

#include "model/io/ImageLoader.h"
#include "model/io/ImageSaver.h"
#include "model/steganography/lsb/LSBEncoder.h"

int main(int argc, char* argv[]) {
    std::string inputPath;
    std::string outputPath;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-i" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        }
    }

    if (inputPath.empty() || outputPath.empty()) {
        std::cerr << "Usage: ./main -i input.jpg -o output.png\n";
        return 1;
    }

    try {
        Image img = ImageLoader::load(inputPath);

        LSBEncoder encoder;
        Image result = encoder.encode(img);

        ImageSaver::savePNG(result, outputPath);

        std::cout << "Done\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}