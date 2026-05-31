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
#include <filesystem>

int main() {
    PipelineConfig config;

    // Pin thread count if set
    std::unique_ptr<tbb::global_control> threadControl;
    if (config.numThreads > 0) {
        threadControl = std::make_unique<tbb::global_control>(
            tbb::global_control::max_allowed_parallelism,
            config.numThreads);
    }

    // Auto-scan input folder
    auto images = config.scanInputFolder(config.inputPath);
    if (images.empty()) {
        std::cerr << "No images found in: " << config.inputPath << "\n";
        return 1;
    }
    std::cout << "Found " << images.size() << " images in "
        << config.inputPath << "\n";

    std::filesystem::create_directories(config.outputPath);

    //   SERIAL BASELINE - (run on first image only)
    std::cout << "\n========== SERIAL BASELINE ("
        << images[0] << ") ==========\n";
    PerformanceTimer serialTimer;

    Image firstImage = ImageLoader::load(images[0]);
    if (firstImage.empty()) return 1;

    serialTimer.start("1. Load image");
    Image grayS = ImageLoader::toGrayscaleSerial(firstImage);
    serialTimer.stop("1. Load image");

    serialTimer.start("3. Edge detection (Sobel)");
    Image edgesS = EdgeDetector::sobelSerial(grayS, config.edgeThreshold);
    serialTimer.stop("3. Edge detection (Sobel)");

    serialTimer.start("4. Hough transform");
    auto accS = HoughTransform::computeSerial(
        edgesS, config.thetaStepDeg, config.rhoStep);
    serialTimer.stop("4. Hough transform");

    serialTimer.start("5. Line detection");
    auto linesS = LineDetector::findLinesSerial(
        accS, config.houghThreshold, config.nmsRadius, config.maxLines);
    serialTimer.stop("5. Line detection");

    serialTimer.printAll();
    double serialTotal = serialTimer.get("3. Edge detection (Sobel)") +
        serialTimer.get("4. Hough transform") +
        serialTimer.get("5. Line detection");


    // Parallel pipeline with 1,2,4,8 threads
    std::cout << "\n========== SCALABILITY ANALYSIS ==========\n";
    std::cout << std::left << std::setw(10) << "Threads"
        << std::setw(16) << "Hough (ms)"
        << std::setw(16) << "Total (ms)"
        << "Speedup\n";
    std::cout << std::string(50, '-') << "\n";

    for (int threads : {1, 2, 4, 8}) {
        tbb::global_control gc(
            tbb::global_control::max_allowed_parallelism, threads);

        Pipeline p(config);
        PipelineData d = p.run(images[0]);

        double total = p.timer().get("3. Edge detection (Sobel)") +
            p.timer().get("4. Hough transform") +
            p.timer().get("5. Line detection");

        std::cout << std::left << std::setw(10) << threads
            << std::setw(16) << std::fixed << std::setprecision(2)
            << p.timer().get("4. Hough transform")
            << std::setw(16) << total
            << std::setprecision(2) << (serialTotal / total) << "x\n";
    }

    //   PARALLEL PIPELINE
    Pipeline pipeline(config);

    for (const auto& imgPath : images) {
        std::cout << "\n========== PIPELINE: " << imgPath
            << " ==========\n";

        PipelineData data = pipeline.run(imgPath);
        if (data.original.empty()) {
            std::cerr << "Skipping " << imgPath << "\n";
            continue;
        }

        const auto& t = pipeline.timer();
        t.printAll();

        double parallelTotal = t.get("3. Edge detection (Sobel)") +
            t.get("4. Hough transform") +
            t.get("5. Line detection");

        std::cout << "\n--- Speedup vs serial ---\n";
        std::cout << "Serial  : " << serialTotal << " ms\n";
        std::cout << "Parallel: " << parallelTotal << " ms\n";
        std::cout << "Speedup : " << (serialTotal / parallelTotal) << "x\n";

        ResultWriter::saveAll(data, t, config.outputPath, serialTotal);
    }

    std::cout << "\nAll done. Results in: " << config.outputPath << "\n";
    return 0;
}