#pragma once
#include "Image.hpp"
#include <vector>

class HoughTransform {
public:
    struct Accumulator {
        std::vector<int> data;
        int rhoCount = 0;
        int thetaCount = 0;
        double rhoMax = 0.0;
        double rhoStep = 1.0;
        double thetaStep = 1.0;

		// Accessor for (rhoIdx, thetaIdx)
        int& at(int rhoIdx, int thetaIdx) {
            return data[rhoIdx * thetaCount + thetaIdx];
        }

		// Const accessor for (rhoIdx, thetaIdx)
        const int& at(int rhoIdx, int thetaIdx) const {
            return data[rhoIdx * thetaCount + thetaIdx];
        }
    };

    // Run Hough transform on binary edge image
    static Accumulator computeSerial(const Image& edges,
        double thetaStepDeg = 1.0,
        double rhoStep = 1.0);

	// Parallel version
    static Accumulator computeParallel(const Image& edges,
        double thetaStepDeg = 1.0,
        double rhoStep = 1.0);
};