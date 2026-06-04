#pragma once
#include "Image.hpp"

class EdgeDetector {
public:
    // Apply Sobel operator to produce binary edge image
    static Image sobelSerial(const Image& gray, int threshold = 50);

	// Parallel version
    static Image sobelParallel(const Image& gray, int threshold = 50);

    // Returns gradient magnitude image
    static Image sobelMagnitudeSerial(const Image& gray);

};