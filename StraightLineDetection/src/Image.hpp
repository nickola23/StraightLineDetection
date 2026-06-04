#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct Image {
    std::vector<uint8_t> data;
    int width = 0;
    int height = 0;
    int channels = 0;           // 1 - grayscale, 3 - RGB, 4 - RGBA

    bool empty() const { return data.empty(); }

    // Get pixel value at (row, col, channel)
    uint8_t at(int row, int col, int ch = 0) const {
        return data[(row * width + col) * channels + ch];
    }

	// Get reference for pixel value at (row, col, channel)
    uint8_t& at(int row, int col, int ch = 0) {
        return data[(row * width + col) * channels + ch];
    }

    int pixelCount() const { return width * height; }
};