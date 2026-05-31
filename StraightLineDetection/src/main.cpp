#include "Image.hpp"
#include "PipelineConfig.hpp"
#include "PerformanceTimer.hpp"
#include "ImageLoader.hpp"
#include "EdgeDetector.hpp"
#include "HoughTransform.hpp"
#include "LineDetector.hpp"
#include "ResultWriter.hpp"
#include <tbb/global_control.h>
#include <iostream>

int main() {
    PipelineConfig config;

    // Pin thread count (0 = TBB decides automatically)
    std::unique_ptr<tbb::global_control> threadControl;
    if (config.numThreads > 0) {
        threadControl = std::make_unique<tbb::global_control>(
            tbb::global_control::max_allowed_parallelism,
            config.numThreads);
    }

    Image original = ImageLoader::load("input/road1.png");
    if (original.empty()) return 1;

    //   SERIAL RUN
    std::cout << "\n========== SERIAL ==========\n";
    PerformanceTimer serialTimer;

    serialTimer.start("1. Load image");
    serialTimer.stop("1. Load image");

    serialTimer.start("2. Grayscale conversion");
    Image grayS = ImageLoader::toGrayscaleSerial(original);
    serialTimer.stop("2. Grayscale conversion");

    serialTimer.start("3. Edge detection (Sobel)");
    Image edgesS = EdgeDetector::sobelSerial(grayS, config.edgeThreshold);
    serialTimer.stop("3. Edge detection (Sobel)");

    serialTimer.start("4. Hough transform");
    auto accS = HoughTransform::computeSerial(
        edgesS, config.thetaStepDeg, config.rhoStep);
    serialTimer.stop("4. Hough transform");

    serialTimer.start("5. Line detection");
    auto linesS = LineDetector::findLines(
        accS, config.houghThreshold, config.nmsRadius, config.maxLines);
    serialTimer.stop("5. Line detection");

    serialTimer.printAll();
    double serialTotal = serialTimer.get("3. Edge detection (Sobel)") +
        serialTimer.get("4. Hough transform") +
        serialTimer.get("5. Line detection");

    //   PARALLEL RUN
    std::cout << "\n========== PARALLEL ==========\n";
    PerformanceTimer parallelTimer;

    parallelTimer.start("1. Load image");
    parallelTimer.stop("1. Load image");

    parallelTimer.start("2. Grayscale conversion");
    Image grayP = ImageLoader::toGrayscaleParallel(original);
    parallelTimer.stop("2. Grayscale conversion");

    parallelTimer.start("3. Edge detection (Sobel)");
    Image edgesP = EdgeDetector::sobelParallel(grayP, config.edgeThreshold);
    parallelTimer.stop("3. Edge detection (Sobel)");

    parallelTimer.start("4. Hough transform");
    auto accP = HoughTransform::computeParallel(
        edgesP, config.thetaStepDeg, config.rhoStep);
    parallelTimer.stop("4. Hough transform");

    parallelTimer.start("5. Line detection");
    auto linesP = LineDetector::findLines(
        accP, config.houghThreshold, config.nmsRadius, config.maxLines);
    parallelTimer.stop("5. Line detection");

    parallelTimer.printAll();
    double parallelTotal = parallelTimer.get("3. Edge detection (Sobel)") +
        parallelTimer.get("4. Hough transform") +
        parallelTimer.get("5. Line detection");

    std::cout << "\n========== SPEEDUP ==========\n";
    std::cout << "Serial total  : " << serialTotal << " ms\n";
    std::cout << "Parallel total: " << parallelTotal << " ms\n";
    std::cout << "Speedup       : " << (serialTotal / parallelTotal) << "x\n";

    //   SAVE OUTPUTS
    ImageLoader::save(grayP, "output/gray.png");
    ImageLoader::save(edgesP, "output/edges.png");

    Image result = ResultWriter::drawLines(original, linesP);
    ImageLoader::save(result, "output/result.png");

    Image accViz = ResultWriter::visualizeAccumulator(accP);
    ImageLoader::save(accViz, "output/accumulator.png");

    ResultWriter::saveReport("output/report.txt",
        linesP, parallelTimer, "test.png", serialTotal);

    return 0;
}