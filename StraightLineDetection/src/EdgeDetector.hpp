#pragma once
#include "Image.hpp"

class EdgeDetector {
public:
    // Apply Sobel operator and threshold to produce binary edge image
    // threshold: 0-255, pixels with gradient magnitude above this are edges
    static Image sobelSerial(const Image& gray, int threshold = 50);

	// Parallel version
    static Image sobelParallel(const Image& gray, int threshold = 50);

    // Returns gradient magnitude image - not thresholded
    static Image sobelMagnitudeSerial(const Image& gray);

};