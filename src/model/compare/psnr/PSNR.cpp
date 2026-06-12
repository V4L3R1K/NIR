#include "PSNR.h"

double PSNR::compare(const Image &img1, const Image &img2)
{
    if (img1.width != img2.width || img1.height != img2.height)
        throw std::runtime_error("PSNR: images must have the same dimensions");

    if (img1.channels != img2.channels)
        throw std::runtime_error("PSNR: images must have the same number of channels");

    size_t totalPixels = static_cast<size_t>(img1.width) * img1.height * img1.channels;

    if (totalPixels == 0)
        throw std::runtime_error("PSNR: empty image");

    double mse = 0.0;

    for (size_t i = 0; i < totalPixels; ++i)
    {
        double diff = static_cast<double>(img1.data[i]) - static_cast<double>(img2.data[i]);
        mse += diff * diff;
    }

    mse /= static_cast<double>(totalPixels);

    if (mse == 0.0)
        return INFINITY; // изображения идентичны

    double maxPixel = 255.0;
    double psnr = 10.0 * std::log10((maxPixel * maxPixel) / mse);

    return psnr;
}