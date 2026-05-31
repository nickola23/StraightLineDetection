#include "Image.hpp"
#include "PipelineConfig.hpp"
#include "PerformanceTimer.hpp"
#include "ImageLoader.hpp"
#include "EdgeDetector.hpp"
#include "HoughTransform.hpp"
#include "LineDetector.hpp"
#include "ResultWriter.hpp"
#include "Pipeline.hpp"
#include <tbb/global_control.h>
#include <iostream>
#include <vector>
#include <string>

int main() {
    PipelineConfig config;

    // Pin thread count if set, otherwise TBB uses all cores
    std::unique_ptr<tbb::global_control> threadControl;
    if (config.numThreads > 0) {
        threadControl = std::make_unique<tbb::global_control>(
            tbb::global_control::max_allowed_parallelism,
            config.numThreads);
    }

    // Test images
    std::vector<std::string> images = {
        "input/road1.png",
        "input/road2.png",
        "input/building1.png",
        "input/building2.png",
    };

    // SERIAL
    std::cout << "\n========== SERIAL ==========\n";
    PerformanceTimer serialTimer;

    Image original = ImageLoader::load(images[0]);
    if (original.empty()) return 1;

    serialTimer.start("1. Load image");
    Image grayS = ImageLoader::toGrayscaleSerial(original);
    serialTimer.stop("1. Load image");

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

    // PARALLEL PIPELINE
    Pipeline pipeline(config);

    for (const auto& imgPath : images) {
        std::cout << "\n========== PIPELINE: " << imgPath << " ==========\n";

        PipelineData data = pipeline.run(imgPath);

        if (data.original.empty()) {
            std::cerr << "Skipping " << imgPath << " — load failed.\n";
            continue;
        }

        // Print results
        std::cout << "Detected " << data.lines.size() << " lines.\n";
        pipeline.timer().printAll();

        // Compute speedup vs serial
        double parallelTotal = pipeline.timer().get("3. Edge detection (Sobel)") +
            pipeline.timer().get("4. Hough transform") +
            pipeline.timer().get("5. Line detection");

        std::cout << "\n--- Speedup vs serial ---\n";
        std::cout << "Serial  : " << serialTotal << " ms\n";
        std::cout << "Parallel: " << parallelTotal << " ms\n";
        std::cout << "Speedup : " << (serialTotal / parallelTotal) << "x\n";

        // Save outputs per image
        std::string baseName = imgPath.substr(imgPath.find_last_of("/\\") + 1);
        std::string stem = baseName.substr(0, baseName.find_last_of('.'));

        ImageLoader::save(data.gray, "output/" + stem + "_gray.png");
        ImageLoader::save(data.edges, "output/" + stem + "_edges.png");

        Image result = ResultWriter::drawLines(data.original, data.lines);
        ImageLoader::save(result, "output/" + stem + "_result.png");

        Image accViz = ResultWriter::visualizeAccumulator(data.accumulator);
        ImageLoader::save(accViz, "output/" + stem + "_accumulator.png");

        ResultWriter::saveReport(
            "output/" + stem + "_report.txt",
            data.lines,
            pipeline.timer(),
            baseName,
            serialTotal);
    }

    return 0;
}