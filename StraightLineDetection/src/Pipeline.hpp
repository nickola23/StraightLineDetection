#pragma once
#include "Image.hpp"
#include "PipelineConfig.hpp"
#include "PerformanceTimer.hpp"
#include "LineDetector.hpp"
#include "HoughTransform.hpp"
#include <string>
#include <vector>
#include <memory>

struct PipelineData {
    std::string imagePath;

    std::shared_ptr<Image>                       original;
    std::shared_ptr<Image>                       gray;
    std::shared_ptr<Image>                       edges;
    std::shared_ptr<HoughTransform::Accumulator> accumulator;
    std::shared_ptr<std::vector<Line>>           lines;

    const Image& getOriginal()    const { return *original; }
    const Image& getGray()        const { return *gray; }
    const Image& getEdges()       const { return *edges; }
    const HoughTransform::Accumulator& getAccumulator() const { return *accumulator; }
    const std::vector<Line>& getLines() const { return *lines; }
};

class Pipeline {
public:
	// Construct pipeline with given configuration
    explicit Pipeline(const PipelineConfig& config);

	// Run the full pipeline on one image path and return all data
    PipelineData run(const std::string& imagePath);

	// Access the timer to get timings for each phase
    const PerformanceTimer& timer() const { return timer_; }

private:
    PipelineConfig   config_;
    PerformanceTimer timer_;
};