#include "HoughTransform.hpp"
#include <cmath>
#include <iostream>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/combinable.h>

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

HoughTransform::Accumulator HoughTransform::computeParallel(
    const Image& edges,
    double thetaStepDeg,
    double rhoStep)
{
    Accumulator acc;
    if (edges.empty() || edges.channels != 1) return acc;

    double diag = std::sqrt((double)edges.width * edges.width +
        (double)edges.height * edges.height);
    acc.rhoMax = diag;
    acc.rhoStep = rhoStep;
    acc.thetaStep = thetaStepDeg;
    acc.rhoCount = static_cast<int>(2.0 * diag / rhoStep) + 1;
    acc.thetaCount = static_cast<int>(180.0 / thetaStepDeg);
    acc.data.assign(acc.rhoCount * acc.thetaCount, 0);

    // Precompute trig tables
    std::vector<double> cosTable(acc.thetaCount);
    std::vector<double> sinTable(acc.thetaCount);
    for (int t = 0; t < acc.thetaCount; ++t) {
        double theta = t * thetaStepDeg * PI / 180.0;
        cosTable[t] = std::cos(theta);
        sinTable[t] = std::sin(theta);
    }

    int totalRows = edges.height;

    // combinable gives each thread a private accumulator
    // No race conditions, no locks — merge at the end
    tbb::combinable<std::vector<int>> localAccs(
        [&]() { return std::vector<int>(acc.rhoCount * acc.thetaCount, 0); }
    );

    tbb::parallel_for(
        tbb::blocked_range<int>(0, totalRows),
        [&](const tbb::blocked_range<int>& range) {
            auto& local = localAccs.local();

            for (int row = range.begin(); row < range.end(); ++row) {
                for (int col = 0; col < edges.width; ++col) {
                    if (edges.at(row, col) == 0) continue;

                    for (int t = 0; t < acc.thetaCount; ++t) {
                        double rho = col * cosTable[t] + row * sinTable[t];
                        int rhoIdx = static_cast<int>((rho + acc.rhoMax) / rhoStep);
                        if (rhoIdx >= 0 && rhoIdx < acc.rhoCount)
                            local[rhoIdx * acc.thetaCount + t]++;
                    }
                }
            }
        }
    );

    // Merge all thread-local accumulators into the final one
    localAccs.combine_each([&](const std::vector<int>& local) {
        for (int i = 0; i < (int)acc.data.size(); ++i)
            acc.data[i] += local[i];
        });

    std::cout << "[HoughTransform] Parallel done.\n";
    return acc;
}