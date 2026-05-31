#include "EdgeDetector.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

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

    // Skip border pixels (1px border) — convolution needs 3x3 neighborhood
    for (int row = 1; row < gray.height - 1; ++row) {
        for (int col = 1; col < gray.width - 1; ++col) {

            int sumX = 0;
            int sumY = 0;

            // Apply 3x3 kernels
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int pixel = gray.at(row + ky, col + kx);
                    sumX += Gx[ky + 1][kx + 1] * pixel;
                    sumY += Gy[ky + 1][kx + 1] * pixel;
                }
            }

            // Gradient magnitude
            int mag = std::abs(sumX) + std::abs(sumY);

            // Clamp to [0, 255]
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
        // Binary threshold: edge pixel = 255, non-edge = 0
        edges.data[i] = (magnitude.data[i] >= threshold) ? 255 : 0;
    }

    std::cout << "[EdgeDetector] Sobel done. Threshold: " << threshold << "\n";
    return edges;
}