#include "SSIM.h"

double SSIM::compare(const Image &img1, const Image &img2)
{
    if (img1.width != img2.width || img1.height != img2.height)
        throw std::runtime_error("SSIM: images must have the same dimensions");

    if (img1.channels != img2.channels)
        throw std::runtime_error("SSIM: images must have the same number of channels");

    int w = img1.width;
    int h = img1.height;
    int c = img1.channels;

    if (w == 0 || h == 0)
        throw std::runtime_error("SSIM: empty image");

    // Константы стабилизации
    double C1 = (0.01 * 255.0) * (0.01 * 255.0);
    double C2 = (0.03 * 255.0) * (0.03 * 255.0);

    // Размер окна и сигма для гауссова ядра
    int windowSize = 11;
    double sigma = 1.5;
    int half = windowSize / 2;

    // Гауссово ядро
    std::vector<std::vector<double>> kernel(windowSize, std::vector<double>(windowSize));
    double sum = 0.0;
    for (int y = -half; y <= half; ++y)
    {
        for (int x = -half; x <= half; ++x)
        {
            double val = std::exp(-(x * x + y * y) / (2.0 * sigma * sigma));
            kernel[y + half][x + half] = val;
            sum += val;
        }
    }
    // Нормализация
    for (int y = 0; y < windowSize; ++y)
        for (int x = 0; x < windowSize; ++x)
            kernel[y][x] /= sum;

    // Для каждого канала SSIM
    double totalSSIM = 0.0;

    for (int ch = 0; ch < c; ++ch)
    {
        double ssimSum = 0.0;
        int count = 0;

        for (int y = half; y < h - half; ++y)
        {
            for (int x = half; x < w - half; ++x)
            {
                double mu1 = 0.0, mu2 = 0.0;
                double sigma1_sq = 0.0, sigma2_sq = 0.0, sigma12 = 0.0;

                // Вычисляем статистики в окне
                for (int wy = -half; wy <= half; ++wy)
                {
                    for (int wx = -half; wx <= half; ++wx)
                    {
                        int px = x + wx;
                        int py = y + wy;
                        double wgt = kernel[wy + half][wx + half];

                        double v1 = static_cast<double>(img1.data[(py * w + px) * c + ch]);
                        double v2 = static_cast<double>(img2.data[(py * w + px) * c + ch]);

                        mu1 += wgt * v1;
                        mu2 += wgt * v2;
                    }
                }

                for (int wy = -half; wy <= half; ++wy)
                {
                    for (int wx = -half; wx <= half; ++wx)
                    {
                        int px = x + wx;
                        int py = y + wy;
                        double wgt = kernel[wy + half][wx + half];

                        double v1 = static_cast<double>(img1.data[(py * w + px) * c + ch]);
                        double v2 = static_cast<double>(img2.data[(py * w + px) * c + ch]);

                        sigma1_sq += wgt * (v1 - mu1) * (v1 - mu1);
                        sigma2_sq += wgt * (v2 - mu2) * (v2 - mu2);
                        sigma12 += wgt * (v1 - mu1) * (v2 - mu2);
                    }
                }

                double numerator = (2.0 * mu1 * mu2 + C1) * (2.0 * sigma12 + C2);
                double denominator = (mu1 * mu1 + mu2 * mu2 + C1) * (sigma1_sq + sigma2_sq + C2);

                ssimSum += numerator / denominator;
                count++;
            }
        }

        totalSSIM += ssimSum / static_cast<double>(count);
    }

    return totalSSIM / static_cast<double>(c);
}