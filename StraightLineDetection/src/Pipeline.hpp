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

    // zero copy transfer between nodes
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
    explicit Pipeline(const PipelineConfig& config);
    PipelineData run(const std::string& imagePath);
    const PerformanceTimer& timer() const { return timer_; }

private:
    PipelineConfig   config_;
    PerformanceTimer timer_;
};