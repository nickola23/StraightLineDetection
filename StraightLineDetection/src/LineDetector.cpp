#include "LineDetector.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

static const double PI = 3.14159265358979323846;

std::vector<Line> LineDetector::findLines(
    const HoughTransform::Accumulator& acc,
    int threshold,
    int nmsRadius,
    int maxLines)
{
    // Step 1: collect all candidates above threshold
    struct Candidate {
        int rhoIdx;
        int thetaIdx;
        int votes;
    };

    std::vector<Candidate> candidates;
    candidates.reserve(512);

    for (int r = 0; r < acc.rhoCount; ++r) {
        for (int t = 0; t < acc.thetaCount; ++t) {
            int v = acc.at(r, t);
            if (v >= threshold) {
                candidates.push_back({ r, t, v });
            }
        }
    }

    // Sort by votes descending — strongest lines first
    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.votes > b.votes;
        });

    // Step 2: for each candidate, suppress all nearby candidates
    std::vector<bool> suppressed(candidates.size(), false);

    for (size_t i = 0; i < candidates.size(); ++i) {
        if (suppressed[i]) continue;

        // Suppress all weaker candidates within nmsRadius window
        for (size_t j = i + 1; j < candidates.size(); ++j) {
            if (suppressed[j]) continue;

            int dRho = std::abs(candidates[i].rhoIdx - candidates[j].rhoIdx);
            int dTheta = std::abs(candidates[i].thetaIdx - candidates[j].thetaIdx);

            // Handle theta wrap-around
            if (dTheta > acc.thetaCount / 2)
                dTheta = acc.thetaCount - dTheta;

            if (dRho <= nmsRadius && dTheta <= nmsRadius)
                suppressed[j] = true;
        }
    }

    // Step 3: convert surviving candidates to Line structs
    std::vector<Line> lines;
    lines.reserve(maxLines);

    for (size_t i = 0; i < candidates.size() && (int)lines.size() < maxLines; ++i) {
        if (suppressed[i]) continue;

        Line line;
        // Convert rho index back to actual rho value
        line.rho = candidates[i].rhoIdx * acc.rhoStep - acc.rhoMax;
        // Convert theta index to radians
        line.theta = candidates[i].thetaIdx * acc.thetaStep * PI / 180.0;
        line.votes = candidates[i].votes;

        lines.push_back(line);
    }

    std::cout << "[LineDetector] Candidates above threshold: " << candidates.size()
        << " | After NMS: " << lines.size() << " lines\n";

    return lines;
}