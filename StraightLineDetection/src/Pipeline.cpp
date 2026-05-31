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
    // Reset timer
    timer_ = PerformanceTimer();

    // The result we will fill progressively
    PipelineData result;
    result.imagePath = imagePath;

    // Build the flow graph
    fg::graph g;

    using namespace tbb::flow;

    // Node 1 — Load and grayscale
    function_node<continue_msg, PipelineData> loadNode(
        g, serial,
        [&](continue_msg) -> PipelineData {
            PipelineData data;
            data.imagePath = imagePath;

            timer_.start("1. Load image");
            data.original = ImageLoader::load(imagePath);
            timer_.stop("1. Load image");

            timer_.start("2. Grayscale conversion");
            data.gray = ImageLoader::toGrayscaleParallel(data.original);
            timer_.stop("2. Grayscale conversion");

            return data;
        }
    );

    // Node 2 — Edge detection
    function_node<PipelineData, PipelineData> edgeNode(
        g, serial,
        [&](PipelineData data) -> PipelineData {
            timer_.start("3. Edge detection (Sobel)");
            data.edges = EdgeDetector::sobelParallel(
                data.gray, config_.edgeThreshold);
            timer_.stop("3. Edge detection (Sobel)");
            return data;
        }
    );

    // Node 3 — Hough transform
    function_node<PipelineData, PipelineData> houghNode(
        g, serial,
        [&](PipelineData data) -> PipelineData {
            timer_.start("4. Hough transform");
            data.accumulator = HoughTransform::computeParallel(
                data.edges, config_.thetaStepDeg, config_.rhoStep);
            timer_.stop("4. Hough transform");
            return data;
        }
    );

    // Node 4 — Line detection
    function_node<PipelineData, PipelineData> lineNode(
        g, serial,
        [&](PipelineData data) -> PipelineData {
            timer_.start("5. Line detection");
            data.lines = LineDetector::findLines(
                data.accumulator,
                config_.houghThreshold,
                config_.nmsRadius,
                config_.maxLines);
            timer_.stop("5. Line detection");
            return data;
        }
    );

    // Node 5 — Collect result - sink node
    function_node<PipelineData> sinkNode(
        g, serial,
        [&](PipelineData data) {
            result = std::move(data);
        }
    );

    // Wire the graph
    make_edge(loadNode, edgeNode);
    make_edge(edgeNode, houghNode);
    make_edge(houghNode, lineNode);
    make_edge(lineNode, sinkNode);

    // Put data in pipeline
    loadNode.try_put(continue_msg());

    // Wait for all nodes to finish
    g.wait_for_all();

    return result;
}