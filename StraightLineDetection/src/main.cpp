#include "Image.hpp"
#include "PipelineConfig.hpp"
#include "PerformanceTimer.hpp"
#include "ImageLoader.hpp"
#include "EdgeDetector.hpp"
#include "HoughTransform.hpp"
#include "LineDetector.hpp"
#include "ResultWriter.hpp"
#include <iostream>

int main() {
    PipelineConfig config;
    PerformanceTimer timer;

    // Phase 1 — Load
    timer.start("1. Load image");
    Image original = ImageLoader::load("input/road2.png");
    timer.stop("1. Load image");
    if (original.empty()) return 1;

    // Phase 1b — Grayscale
    timer.start("2. Grayscale conversion");
    Image gray = ImageLoader::toGrayscaleSerial(original);
    timer.stop("2. Grayscale conversion");

    // Phase 2 — Edges
    timer.start("3. Edge detection (Sobel)");
    Image edges = EdgeDetector::sobelSerial(gray, config.edgeThreshold);
    timer.stop("3. Edge detection (Sobel)");

    // Phase 3 — Hough
    timer.start("4. Hough transform");
    auto acc = HoughTransform::computeSerial(
        edges, config.thetaStepDeg, config.rhoStep);
    timer.stop("4. Hough transform");

    // Phase 4 — Line detection
    timer.start("5. Line detection");
    std::vector<Line> lines = LineDetector::findLines(
        acc,
        config.houghThreshold,
        config.nmsRadius,
        config.maxLines);
    timer.stop("5. Line detection");

    // --- Save all outputs ---
    ImageLoader::save(gray, "output/gray.png");
    ImageLoader::save(edges, "output/edges.png");

    Image result = ResultWriter::drawLines(original, lines);
    ImageLoader::save(result, "output/result.png");

    Image accViz = ResultWriter::visualizeAccumulator(acc);
    ImageLoader::save(accViz, "output/accumulator.png");

    ResultWriter::saveReport("output/report.txt", lines, timer, "test.png");

    timer.printAll();
    return 0;
}