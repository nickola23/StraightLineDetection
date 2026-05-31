#pragma once
#include <string>

struct PipelineConfig {

    // --- Paths ---
    std::string inputPath = "input/";
    std::string outputPath = "output/";

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
};