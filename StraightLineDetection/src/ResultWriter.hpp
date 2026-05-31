#pragma once
#include "Image.hpp"
#include "LineDetector.hpp"
#include "HoughTransform.hpp"
#include "PerformanceTimer.hpp"
#include "Pipeline.hpp"
#include <vector>
#include <string>
#include <filesystem>

class ResultWriter {
public:
    static Image drawLines(const Image& original,
        const std::vector<Line>& lines,
        uint8_t r = 255,
        uint8_t g = 0,
        uint8_t b = 0);

    static void saveReport(const std::string& path,
        const std::vector<Line>& lines,
        const PerformanceTimer& timer,
        const std::string& imageName,
        double serialTotalMs = -1.0);

    // Visualize accumulator as a grayscale image (for documentation)
    static Image visualizeAccumulator(const HoughTransform::Accumulator& acc);

    // Save all outputs for one pipeline result into its own subfolder
    static void saveAll(const PipelineData& data,
        const PerformanceTimer& timer,
        const std::string& outputRoot,
        double serialTotalMs = -1.0);
};