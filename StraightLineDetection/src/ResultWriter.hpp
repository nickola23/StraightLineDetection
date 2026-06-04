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
	// Draw detected lines and return new image
    static Image drawLines(const Image& original,
        const std::vector<Line>& lines,
        uint8_t r = 255,
        uint8_t g = 0,
        uint8_t b = 0);

	// Save a text report with lines and timings
    static void saveReport(const std::string& path,
        const std::vector<Line>& lines,
        const PerformanceTimer& timer,
        const std::string& imageName,
        double serialTotalMs = -1.0);

    // Visualize accumulator as a grayscale image
    static Image visualizeAccumulator(const HoughTransform::Accumulator& acc);

    // Save all outputs for one pipeline into its own subfolder
    static void saveAll(const PipelineData& data,
        const PerformanceTimer& timer,
        const std::string& outputRoot,
        double serialTotalMs = -1.0);
};