#include "SSIM.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace
{
    constexpr int WINDOW_SIZE = 11;
    constexpr int HALF = WINDOW_SIZE / 2;

    inline std::size_t idx(int x, int y, int w)
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x);
    }

    std::array<float, WINDOW_SIZE> makeGaussian1D(double sigma)
    {
        std::array<float, WINDOW_SIZE> kernel{};
        double sum = 0.0;

        for (int i = -HALF; i <= HALF; ++i)
        {
            double v = std::exp(-(i * i) / (2.0 * sigma * sigma));
            kernel[i + HALF] = static_cast<float>(v);
            sum += v;
        }

        for (float &v : kernel)
        {
            v = static_cast<float>(v / sum);
        }

        return kernel;
    }

    void gaussianBlurValid(
        const std::vector<float> &src,
        std::vector<float> &dst,
        std::vector<float> &scratch,
        int w,
        int h,
        const std::array<float, WINDOW_SIZE> &kernel)
    {
        const std::size_t size = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);

        scratch.assign(size, 0.0f);
        dst.assign(size, 0.0f);

        if (w < WINDOW_SIZE || h < WINDOW_SIZE)
        {
            return;
        }

// Горизонтальный проход
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if (h >= 128)
#endif
        for (int y = 0; y < h; ++y)
        {
            for (int x = HALF; x < w - HALF; ++x)
            {
                double sum = 0.0;
                const std::size_t base = idx(x, y, w);

                for (int k = -HALF; k <= HALF; ++k)
                {
                    sum += static_cast<double>(kernel[k + HALF]) *
                           static_cast<double>(src[base + k]);
                }

                scratch[base] = static_cast<float>(sum);
            }
        }

// Вертикальный проход
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if (h >= 128)
#endif
        for (int y = HALF; y < h - HALF; ++y)
        {
            for (int x = HALF; x < w - HALF; ++x)
            {
                double sum = 0.0;
                const std::size_t base = idx(x, y, w);

                for (int k = -HALF; k <= HALF; ++k)
                {
                    sum += static_cast<double>(kernel[k + HALF]) *
                           static_cast<double>(scratch[idx(x, y + k, w)]);
                }

                dst[base] = static_cast<float>(sum);
            }
        }
    }
}

double SSIM::compare(const Image &img1, const Image &img2)
{
    if (img1.width != img2.width || img1.height != img2.height)
        throw std::runtime_error("SSIM: images must have the same dimensions");

    if (img1.channels != img2.channels)
        throw std::runtime_error("SSIM: images must have the same number of channels");

    const int w = img1.width;
    const int h = img1.height;
    const int c = img1.channels;

    if (w <= 0 || h <= 0)
        throw std::runtime_error("SSIM: empty image");

    if (w < WINDOW_SIZE || h < WINDOW_SIZE)
        throw std::runtime_error("SSIM: image must be at least 11x11 for SSIM window");

    const double C1 = (0.01 * 255.0) * (0.01 * 255.0);
    const double C2 = (0.03 * 255.0) * (0.03 * 255.0);

    const auto kernel = makeGaussian1D(1.5);

    const std::size_t size = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);

    std::vector<float> plane1(size);
    std::vector<float> plane2(size);
    std::vector<float> plane1_sq(size);
    std::vector<float> plane2_sq(size);
    std::vector<float> plane12(size);

    std::vector<float> mu1(size);
    std::vector<float> mu2(size);
    std::vector<float> e11(size);
    std::vector<float> e22(size);
    std::vector<float> e12(size);

    std::vector<float> scratch;

    const int validW = w - 2 * HALF;
    const int validH = h - 2 * HALF;
    const std::size_t validCount = static_cast<std::size_t>(validW) * static_cast<std::size_t>(validH);

    double totalSSIM = 0.0;

    for (int ch = 0; ch < c; ++ch)
    {
// Сбор одного канала в плоские массивы
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if (size >= 1 << 20)
#endif
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(size); ++i)
        {
            const std::size_t p = static_cast<std::size_t>(i) * static_cast<std::size_t>(c) + static_cast<std::size_t>(ch);
            const float a = static_cast<float>(img1.data[p]);
            const float b = static_cast<float>(img2.data[p]);

            plane1[static_cast<std::size_t>(i)] = a;
            plane2[static_cast<std::size_t>(i)] = b;
            plane1_sq[static_cast<std::size_t>(i)] = a * a;
            plane2_sq[static_cast<std::size_t>(i)] = b * b;
            plane12[static_cast<std::size_t>(i)] = a * b;
        }

        // Гауссовы свёртки
        gaussianBlurValid(plane1, mu1, scratch, w, h, kernel);
        gaussianBlurValid(plane2, mu2, scratch, w, h, kernel);
        gaussianBlurValid(plane1_sq, e11, scratch, w, h, kernel);
        gaussianBlurValid(plane2_sq, e22, scratch, w, h, kernel);
        gaussianBlurValid(plane12, e12, scratch, w, h, kernel);

        double channelSSIM = 0.0;

#ifdef _OPENMP
#pragma omp parallel for reduction(+ : channelSSIM) schedule(static) if (validH >= 128)
#endif
        for (int y = HALF; y < h - HALF; ++y)
        {
            for (int x = HALF; x < w - HALF; ++x)
            {
                const std::size_t p = idx(x, y, w);

                const double m1 = static_cast<double>(mu1[p]);
                const double m2 = static_cast<double>(mu2[p]);

                const double s1_sq = static_cast<double>(e11[p]) - m1 * m1;
                const double s2_sq = static_cast<double>(e22[p]) - m2 * m2;
                const double s12 = static_cast<double>(e12[p]) - m1 * m2;

                const double numerator = (2.0 * m1 * m2 + C1) * (2.0 * s12 + C2);
                const double denominator = (m1 * m1 + m2 * m2 + C1) * (s1_sq + s2_sq + C2);

                channelSSIM += numerator / denominator;
            }
        }

        totalSSIM += channelSSIM / static_cast<double>(validCount);
    }

    return totalSSIM / static_cast<double>(c);
}