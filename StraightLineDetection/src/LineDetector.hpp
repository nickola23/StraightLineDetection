#pragma once
#include "HoughTransform.hpp"
#include <vector>

struct Line {
    double rho = 0.0;
    double theta = 0.0;
    int    votes = 0;
};

class LineDetector {
public:
    // Find lines in accumulator above threshold
    static std::vector<Line> findLinesSerial(
        const HoughTransform::Accumulator& acc,
        int threshold,
        int nmsRadius = 10,
        int maxLines = 50);

	// Parallel version
    static std::vector<Line> findLinesParallel(
        const HoughTransform::Accumulator& acc,
        int threshold,
        int nmsRadius = 10,
        int maxLines = 50);
};