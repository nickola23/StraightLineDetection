#include "Pipeline.hpp"
#include "ImageLoader.hpp"
#include "EdgeDetector.hpp"
#include "HoughTransform.hpp"
#include "LineDetector.hpp"
#include <tbb/flow_graph.h>
#include <iostream>

namespace fg = tbb::flow;

Pipeline::Pipeline(const PipelineConfig& config)
    : config_(config) {
}

PipelineData Pipeline::run(const std::string& imagePath)
{
    timer_ = PerformanceTimer();

    PipelineData result;
    result.imagePath = imagePath;

    fg::graph g;

    // Node 1 — Load and grayscale
    fg::function_node<fg::continue_msg, PipelineData> loadNode(
        g, fg::serial,
        [&](fg::continue_msg) -> PipelineData {
            PipelineData data;
            data.imagePath = imagePath;

            timer_.start("1. Load image");
            data.original = std::make_shared<Image>(
                ImageLoader::load(imagePath));
            timer_.stop("1. Load image");

            timer_.start("2. Grayscale conversion");
            data.gray = std::make_shared<Image>(
                ImageLoader::toGrayscaleParallel(*data.original));
            timer_.stop("2. Grayscale conversion");

            return data;
        }
    );

    // Node 2 — Edge detection
    fg::function_node<PipelineData, PipelineData> edgeNode(
        g, fg::serial,
        [&](PipelineData data) -> PipelineData {
            timer_.start("3. Edge detection (Sobel)");
            data.edges = std::make_shared<Image>(
                EdgeDetector::sobelParallel(*data.gray, config_.edgeThreshold));
            timer_.stop("3. Edge detection (Sobel)");
            return data;
        }
    );

    // Node 3 — Hough transform
    fg::function_node<PipelineData, PipelineData> houghNode(
        g, fg::serial,
        [&](PipelineData data) -> PipelineData {
            timer_.start("4. Hough transform");
            data.accumulator = std::make_shared<HoughTransform::Accumulator>(
                HoughTransform::computeParallel(
                    *data.edges, config_.thetaStepDeg, config_.rhoStep));
            timer_.stop("4. Hough transform");
            return data;
        }
    );

    // Node 4 — Line detection
    fg::function_node<PipelineData, PipelineData> lineNode(
        g, fg::serial,
        [&](PipelineData data) -> PipelineData {
            timer_.start("5. Line detection");
            data.lines = std::make_shared<std::vector<Line>>(
                LineDetector::findLinesParallel(
                    *data.accumulator,
                    config_.houghThreshold,
                    config_.nmsRadius,
                    config_.maxLines));
            timer_.stop("5. Line detection");
            return data;
        }
    );

    // Node 5 — Sink
    fg::function_node<PipelineData> sinkNode(
        g, fg::serial,
        [&](PipelineData data) {
            result = std::move(data);
        }
    );

    fg::make_edge(loadNode, edgeNode);
    fg::make_edge(edgeNode, houghNode);
    fg::make_edge(houghNode, lineNode);
    fg::make_edge(lineNode, sinkNode);

    loadNode.try_put(fg::continue_msg());
    g.wait_for_all();

    return result;
}