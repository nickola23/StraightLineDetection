#pragma once
#include <string>
#include <vector>
#include <filesystem>

struct PipelineConfig {

    // --- Paths ---
    std::string inputPath = "input/";
    std::string outputPath = "output";

    // --- Edge detection ---
    int    edgeThreshold = 190;      // Sobel magnitude threshold (0-255)

    // --- Hough transform ---
    int    houghThreshold = 185;    // Min accumulator votes to count as a line
    double thetaStepDeg = 1.0;      // Angular resolution in degrees
    double rhoStep = 2.0;           // Distance resolution in pixels

    // --- Line detection ---
    int    maxLines = 10;           // Max lines to extract
    int    nmsRadius = 20;          // Non-maximum suppression radius in accumulator

    // --- Parallelism ---
    int    numThreads = 0;          // 0 = TBB automatic uses all cores

    // --- Output ---
    bool   saveEdgeImage = true;
    bool   saveResultImage = true;
    bool   printTiming = true;

    // Scan input folder and return all image paths
    inline std::vector<std::string> scanInputFolder(const std::string& folder) {
        std::vector<std::string> paths;
        for (auto& entry : std::filesystem::directory_iterator(folder)) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" ||
                ext == ".jpeg" || ext == ".bmp")
                paths.push_back(entry.path().string());
        }
        std::sort(paths.begin(), paths.end());
        return paths;
    }
};