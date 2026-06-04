#include "LineDetector.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/concurrent_vector.h>

static const double PI = 3.14159265358979323846;

std::vector<Line> LineDetector::findLinesSerial(
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

	// Step 2: sort by votes descending
    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.votes > b.votes;
        });

    // Step 3: for each candidate, suppress all nearby candidates
    std::vector<bool> suppressed(candidates.size(), false);

    for (size_t i = 0; i < candidates.size(); ++i) {
        if (suppressed[i]) continue;

        for (size_t j = i + 1; j < candidates.size(); ++j) {
            if (suppressed[j]) continue;

            int dRho = std::abs(candidates[i].rhoIdx - candidates[j].rhoIdx);
            int dTheta = std::abs(candidates[i].thetaIdx - candidates[j].thetaIdx);

            if (dTheta > acc.thetaCount / 2)
                dTheta = acc.thetaCount - dTheta;

            if (dRho <= nmsRadius && dTheta <= nmsRadius)
                suppressed[j] = true;
        }
    }

    // Step 4: convert to Line structs
    std::vector<Line> lines;
    lines.reserve(maxLines);

    for (size_t i = 0; i < candidates.size() && (int)lines.size() < maxLines; ++i) {
        if (suppressed[i]) continue;

        Line line;
        line.rho = candidates[i].rhoIdx * acc.rhoStep - acc.rhoMax;
        line.theta = candidates[i].thetaIdx * acc.thetaStep * PI / 180.0;
        line.votes = candidates[i].votes;

        lines.push_back(line);
    }

    std::cout << "[LineDetector] Candidates above threshold: " << candidates.size()
        << " | After NMS: " << lines.size() << " lines\n";

    return lines;
}

std::vector<Line> LineDetector::findLinesParallel(
    const HoughTransform::Accumulator& acc,
    int threshold,
    int nmsRadius,
    int maxLines)
{
    // Step 1: check if its maximum and above threshold
    tbb::concurrent_vector<std::pair<int, int>> maxima;

    tbb::parallel_for(
        tbb::blocked_range2d<int>(0, acc.rhoCount, 0, acc.thetaCount),
        [&](const tbb::blocked_range2d<int>& range) {
            for (int r = range.rows().begin(); r < range.rows().end(); ++r) {
                for (int t = range.cols().begin(); t < range.cols().end(); ++t) {
                    int v = acc.at(r, t);
                    if (v < threshold) continue;

                    bool isMax = true;
                    int rLo = std::max(0, r - nmsRadius);
                    int rHi = std::min(acc.rhoCount - 1, r + nmsRadius);
                    int tLo = std::max(0, t - nmsRadius);
                    int tHi = std::min(acc.thetaCount - 1, t + nmsRadius);

                    for (int nr = rLo; nr <= rHi && isMax; ++nr)
                        for (int nt = tLo; nt <= tHi && isMax; ++nt)
                            if (acc.at(nr, nt) > v) isMax = false;

                    if (isMax)
                        maxima.push_back({ v, r * acc.thetaCount + t });
                }
            }
        }
    );

    // Step 2: sort by votes descending
    std::vector<std::pair<int, int>> sorted(maxima.begin(), maxima.end());
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    if ((int)sorted.size() > maxLines)
        sorted.resize(maxLines);

    // Step 3: convert to Line structs
    std::vector<Line> lines;
    lines.reserve(sorted.size());

    for (auto& [votes, idx] : sorted) {
        int rhoIdx = idx / acc.thetaCount;
        int thetaIdx = idx % acc.thetaCount;

        Line line;
        line.rho = rhoIdx * acc.rhoStep - acc.rhoMax;
        line.theta = thetaIdx * acc.thetaStep * PI / 180.0;
        line.votes = votes;
        lines.push_back(line);
    }

    std::cout << "[LineDetector] Local maxima found: " << maxima.size()
        << " | Returning top: " << lines.size() << " lines\n";

    return lines;
}