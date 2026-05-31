#pragma once
#include "Image.hpp"
#include "PipelineConfig.hpp"
#include <string>

class ImageLoader {
public:
    // Load image from file - BMP, PNG, JPG, PPM
    // Returns empty Image on failure
    static Image load(const std::string& path);

    // Convert RGB/RGBA image to grayscale (serial version)
    static Image toGrayscaleSerial(const Image& src);

    // Save image to file - PNG output
    static bool save(const Image& img, const std::string& path);
};