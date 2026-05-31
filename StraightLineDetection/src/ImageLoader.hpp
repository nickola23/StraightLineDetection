#pragma once
#include "Image.hpp"
#include "PipelineConfig.hpp"
#include <string>

class ImageLoader {
public:
    // Load image from file - BMP, PNG, JPG, PPM
    static Image load(const std::string& path);

    // Convert RGB/RGBA image to grayscale - serial version
    static Image toGrayscaleSerial(const Image& src);

	// Convert RGB/RGBA image to grayscale - parallel version
    static Image toGrayscaleParallel(const Image& src);

    // Save image to file - PNG output
    static bool save(const Image& img, const std::string& path);
};