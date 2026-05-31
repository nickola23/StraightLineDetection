#pragma once
#include "HoughTransform.hpp"
#include <vector>

struct Line {
    double rho = 0.0;       // distance from origin
    double theta = 0.0;     // angle in radians
    int    votes = 0;       // accumulator strength
};

class LineDetector {
public:
    // Find lines in accumulator above threshold
    // threshold : min votes to be considered a line
    // nmsRadius : suppression window — prevents duplicate nearby lines
    // maxLines  : cap on how many lines to return (strongest first)
    static std::vector<Line> findLinesSerial(
        const HoughTransform::Accumulator& acc,
        int threshold,
        int nmsRadius = 10,
        int maxLines = 50);

    static std::vector<Line> findLinesParallel(
        const HoughTransform::Accumulator& acc,
        int threshold,
        int nmsRadius = 10,
        int maxLines = 50);
};