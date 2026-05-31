#pragma once
#include "Image.hpp"
#include "PipelineConfig.hpp"
#include "PerformanceTimer.hpp"
#include "LineDetector.hpp"
#include "HoughTransform.hpp"
#include <string>
#include <vector>

// Data packet that flows through the graph
struct PipelineData {
    std::string     imagePath;
    Image           original;
    Image           gray;
    Image           edges;
    HoughTransform::Accumulator accumulator;
    std::vector<Line>           lines;
};

class Pipeline {
public:
    explicit Pipeline(const PipelineConfig& config);

    // Run the full parallel pipeline on a single image
    PipelineData run(const std::string& imagePath);

    const PerformanceTimer& timer() const { return timer_; }

private:
    PipelineConfig  config_;
    PerformanceTimer timer_;
};