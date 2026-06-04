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
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>

// Returns total compute time for a serial run
double runSerial(const Image& original, const PipelineConfig& config,
    PerformanceTimer& timer)
{
    timer = PerformanceTimer();

    timer.start("2. Grayscale conversion");
    Image gray = ImageLoader::toGrayscaleSerial(original);
    timer.stop("2. Grayscale conversion");

    timer.start("3. Edge detection (Sobel)");
    Image edges = EdgeDetector::sobelSerial(gray, config.edgeThreshold);
    timer.stop("3. Edge detection (Sobel)");

    timer.start("4. Hough transform");
    auto acc = HoughTransform::computeSerial(
        edges, config.thetaStepDeg, config.rhoStep);
    timer.stop("4. Hough transform");

    timer.start("5. Line detection");
    LineDetector::findLinesParallel(
        acc, config.houghThreshold, config.nmsRadius, config.maxLines);
    timer.stop("5. Line detection");

    return timer.get("2. Grayscale conversion") +
        timer.get("3. Edge detection (Sobel)") +
        timer.get("4. Hough transform") +
        timer.get("5. Line detection");
}

int main() {
    PipelineConfig config;
    auto images = config.scanInputFolder(config.inputPath);
    if (images.empty()) {
        std::cerr << "No images found in: " << config.inputPath << "\n";
        return 1;
    }
    std::cout << "Found " << images.size() << " images in "
        << config.inputPath << "\n";
    std::filesystem::create_directories(config.outputPath);

    std::cout << "\n========== SCALABILITY ANALYSIS ("
        << images[0] << ") ==========\n";

    PerformanceTimer scaleSerialTimer;
    Image firstImage = ImageLoader::load(images[0]);
    if (firstImage.empty()) return 1;
    double scaleSerialMs = runSerial(firstImage, config, scaleSerialTimer);

    std::cout << "\nSerial compute baseline: "
        << std::fixed << std::setprecision(2)
        << scaleSerialMs << " ms\n\n";

    std::cout << std::left
        << std::setw(10) << "Threads"
        << std::setw(20) << "Hough (ms)"
        << std::setw(24) << "Compute Time (ms)"
        << "Speedup vs Serial\n";
    std::cout << std::string(68, '-') << "\n";

    for (int threads : {1, 2, 4, 8}) {
        tbb::global_control gc(
            tbb::global_control::max_allowed_parallelism, threads);

        Pipeline p(config);
        PipelineData d = p.run(images[0]);

        double computeMs = p.timer().get("2. Grayscale conversion") +
            p.timer().get("3. Edge detection (Sobel)") +
            p.timer().get("4. Hough transform") +
            p.timer().get("5. Line detection");

        std::cout << std::left
            << std::setw(10) << threads
            << std::setw(20) << std::fixed << std::setprecision(2)
            << p.timer().get("4. Hough transform")
            << std::setw(24) << computeMs
            << std::setprecision(2) << (scaleSerialMs / computeMs)
            << "x\n";
    }

    // PER-IMAGE PIPELINE
    Pipeline pipeline(config);

    for (const auto& imgPath : images) {
        std::cout << "\n========== IMAGE: " << imgPath << " ==========\n";

        Image img = ImageLoader::load(imgPath);
        if (img.empty()) {
            std::cerr << "Skipping " << imgPath << "\n";
            continue;
        }

        std::cout << "\n  [Serial]\n";
        PerformanceTimer serialTimer;
        double serialComputeMs = runSerial(img, config, serialTimer);
        serialTimer.printAll();

        std::cout << "\n  [Parallel - flow_graph]\n";
        PipelineData data = pipeline.run(imgPath);
        if (data.getOriginal().empty()) {
            std::cerr << "Skipping parallel run for " << imgPath << "\n";
            continue;
        }
        pipeline.timer().printAll();

        double parallelComputeMs =
            pipeline.timer().get("2. Grayscale conversion") +
            pipeline.timer().get("3. Edge detection (Sobel)") +
            pipeline.timer().get("4. Hough transform") +
            pipeline.timer().get("5. Line detection");

        double pipelineTotalMs =
            pipeline.timer().get("1. Load image") +
            parallelComputeMs;

        std::cout << "\n  --- Results ---\n";
        std::cout << "  Detected lines              : "
            << data.getLines().size() << "\n";
        std::cout << "  Serial compute time         : "
            << std::fixed << std::setprecision(2)
            << serialComputeMs << " ms  (excl. I/O)\n";
        std::cout << "  Parallel compute time       : "
            << parallelComputeMs << " ms  (excl. I/O)\n";
        std::cout << "  Total pipeline time         : "
            << pipelineTotalMs << " ms  (incl. I/O)\n";
        std::cout << "  Speedup (compute only)      : "
            << (serialComputeMs / parallelComputeMs) << "x\n";

        ResultWriter::saveAll(data, pipeline.timer(),
            config.outputPath, serialComputeMs);
    }

    std::cout << "\nAll done. Results in: " << config.outputPath << "\n";
    return 0;
}