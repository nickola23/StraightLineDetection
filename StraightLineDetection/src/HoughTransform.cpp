#include "HoughTransform.hpp"
#include <cmath>
#include <iostream>

static const double PI = 3.14159265358979323846;

HoughTransform::Accumulator HoughTransform::computeSerial(
    const Image& edges,
    double thetaStepDeg,
    double rhoStep)
{
    Accumulator acc;

    if (edges.empty() || edges.channels != 1) {
        std::cerr << "[HoughTransform] Input must be a binary edge image.\n";
        return acc;
    }

    double diag = std::sqrt(
        static_cast<double>(edges.width) * edges.width +
        static_cast<double>(edges.height) * edges.height
    );

    acc.rhoMax = diag;
    acc.rhoStep = rhoStep;
    acc.thetaStep = thetaStepDeg;

    // Number of bins
    acc.rhoCount = static_cast<int>(2.0 * diag / rhoStep) + 1;
    acc.thetaCount = static_cast<int>(180.0 / thetaStepDeg);

    acc.data.assign(acc.rhoCount * acc.thetaCount, 0);

    // Precompute cos/sin tables — avoids recomputing inside the hot loop
    std::vector<double> cosTable(acc.thetaCount);
    std::vector<double> sinTable(acc.thetaCount);
    for (int t = 0; t < acc.thetaCount; ++t) {
        double theta = t * thetaStepDeg * PI / 180.0;
        cosTable[t] = std::cos(theta);
        sinTable[t] = std::sin(theta);
    }

    // Vote
    int edgeCount = 0;
    for (int row = 0; row < edges.height; ++row) {
        for (int col = 0; col < edges.width; ++col) {

            // Only process edge pixels
            if (edges.at(row, col) == 0) continue;
            ++edgeCount;

            for (int t = 0; t < acc.thetaCount; ++t) {
                // Compute rho for this (x, y, theta)
                double rho = col * cosTable[t] + row * sinTable[t];

                // Map rho to accumulator index
                int rhoIdx = static_cast<int>((rho + acc.rhoMax) / rhoStep);

                // Bounds check
                if (rhoIdx >= 0 && rhoIdx < acc.rhoCount) {
                    acc.at(rhoIdx, t)++;
                }
            }
        }
    }

    std::cout << "[HoughTransform] Done. Edge pixels voted: " << edgeCount
        << " | Accumulator size: " << acc.rhoCount
        << " x " << acc.thetaCount << "\n";

    return acc;
}