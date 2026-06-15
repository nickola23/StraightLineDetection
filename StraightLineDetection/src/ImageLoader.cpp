#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../third_party/stb_image.h"
#include "../third_party/stb_image_write.h"

#include "ImageLoader.hpp"
#include <iostream>
#include <stdexcept>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

Image ImageLoader::load(const std::string& path) {
    Image img;
    int w, h, ch;

    unsigned char* rawData = stbi_load(path.c_str(), &w, &h, &ch, 3);

    if (!rawData) {
        std::cerr << "[ImageLoader] Failed to load: " << path
            << " — " << stbi_failure_reason() << "\n";
        return img;
    }

    img.width = w;
    img.height = h;
    img.channels = 3;
    img.data.assign(rawData, rawData + w * h * 3);

    stbi_image_free(rawData);

    std::cout << "[ImageLoader] Loaded: " << path
        << " (" << w << "x" << h << ", " << ch << " ch original)\n";
    return img;
}

Image ImageLoader::toGrayscaleSerial(const Image& src) {
    if (src.empty()) return {};
    if (src.channels == 1) return src;

    Image gray;
    gray.width = src.width;
    gray.height = src.height;
    gray.channels = 1;
    gray.data.resize(src.width * src.height);

    for (int i = 0; i < src.width * src.height; ++i) {
        uint8_t r = src.data[i * 3 + 0];
        uint8_t g = src.data[i * 3 + 1];
        uint8_t b = src.data[i * 3 + 2];
        gray.data[i] = static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b);
    }

    return gray;
}

Image ImageLoader::toGrayscaleParallel(const Image& src) {
    if (src.empty()) return {};
    if (src.channels == 1) return src;

    Image gray;
    gray.width = src.width;
    gray.height = src.height;
    gray.channels = 1;
    gray.data.resize(src.width * src.height);

    int total = src.width * src.height;

    tbb::parallel_for(
        tbb::blocked_range<int>(0, total),
        [&](const tbb::blocked_range<int>& range) {
            for (int i = range.begin(); i < range.end(); ++i) {
                uint8_t r = src.data[i * 3 + 0];
                uint8_t g = src.data[i * 3 + 1];
                uint8_t b = src.data[i * 3 + 2];
                gray.data[i] = static_cast<uint8_t>(
                    0.299f * r + 0.587f * g + 0.114f * b);
            }
        }
    );

    return gray;
}

bool ImageLoader::save(const Image& img, const std::string& path) {
    if (img.empty()) {
        std::cerr << "[ImageLoader] Cannot save empty image to: " << path << "\n";
        return false;
    }

    int result = stbi_write_png(
        path.c_str(),
        img.width,
        img.height,
        img.channels,
        img.data.data(),
        img.width * img.channels
    );

    if (!result) {
        std::cerr << "[ImageLoader] Failed to save: " << path << "\n";
        return false;
    }

    std::cout << "[ImageLoader] Saved: " << path << "\n";
    return true;
}