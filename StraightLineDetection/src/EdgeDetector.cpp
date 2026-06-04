#include "EdgeDetector.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

// Gx detects horizontal edges
static const int Gx[3][3] = {
    {-1,  0,  1},
    {-2,  0,  2},
    {-1,  0,  1}
};

// Gy detects vertical edges
static const int Gy[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

Image EdgeDetector::sobelMagnitudeSerial(const Image& gray) {
    if (gray.empty() || gray.channels != 1) {
        std::cerr << "[EdgeDetector] Input must be a grayscale image.\n";
        return {};
    }

    Image magnitude;
    magnitude.width = gray.width;
    magnitude.height = gray.height;
    magnitude.channels = 1;
    magnitude.data.resize(gray.width * gray.height, 0);

    for (int row = 1; row < gray.height - 1; ++row) {
        for (int col = 1; col < gray.width - 1; ++col) {

            int sumX = 0;
            int sumY = 0;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int pixel = gray.at(row + ky, col + kx);
                    sumX += Gx[ky + 1][kx + 1] * pixel;
                    sumY += Gy[ky + 1][kx + 1] * pixel;
                }
            }

            int mag = std::abs(sumX) + std::abs(sumY);

            magnitude.at(row, col) = static_cast<uint8_t>(std::min(mag, 255));
        }
    }

    return magnitude;
}

Image EdgeDetector::sobelSerial(const Image& gray, int threshold) {
    Image magnitude = sobelMagnitudeSerial(gray);
    if (magnitude.empty()) return {};

    Image edges;
    edges.width = gray.width;
    edges.height = gray.height;
    edges.channels = 1;
    edges.data.resize(gray.width * gray.height, 0);

    for (int i = 0; i < gray.width * gray.height; ++i) {
        edges.data[i] = (magnitude.data[i] >= threshold) ? 255 : 0;
    }

    std::cout << "[EdgeDetector] Sobel done. Threshold: " << threshold << "\n";
    return edges;
}

Image EdgeDetector::sobelParallel(const Image& gray, int threshold) {
    if (gray.empty() || gray.channels != 1) return {};

    Image edges;
    edges.width = gray.width;
    edges.height = gray.height;
    edges.channels = 1;
    edges.data.resize(gray.width * gray.height, 0);

    tbb::parallel_for(
        tbb::blocked_range2d<int>(1, gray.height - 1, 1, gray.width - 1),
        [&](const tbb::blocked_range2d<int>& range) {
            for (int row = range.rows().begin(); row < range.rows().end(); ++row) {
                for (int col = range.cols().begin(); col < range.cols().end(); ++col) {

                    int sumX = 0, sumY = 0;
                    for (int ky = -1; ky <= 1; ++ky) {
                        for (int kx = -1; kx <= 1; ++kx) {
                            int pixel = gray.at(row + ky, col + kx);
                            sumX += Gx[ky + 1][kx + 1] * pixel;
                            sumY += Gy[ky + 1][kx + 1] * pixel;
                        }
                    }

                    int mag = std::abs(sumX) + std::abs(sumY);
                    edges.at(row, col) = (mag >= threshold) ? 255 : 0;
                }
            }
        }
    );

    std::cout << "[EdgeDetector] Sobel parallel done. Threshold: "
        << threshold << "\n";
    return edges;
}