#pragma once
#include <string>
#include <vector>

struct PipelineConfig {

    std::string inputPath = "input/";
    std::string outputPath = "output";

    int    edgeThreshold = 190;         // Sobel magnitude threshold (0-255)

    int    houghThreshold = 185;        // Min accumulator votes to count as a line
    double thetaStepDeg = 1.0;          // Angular resolution in degrees
    double rhoStep = 2.0;               // Distance resolution in pixels

    int    maxLines = 6;                // Max lines to extract
    int    nmsRadius = 20;              // Non-maximum suppression radius in accumulator

    int    numThreads = 0;              // 0 = TBB automatic uses all cores

	bool   saveEdgeImage = true;        // Save binary edge image
	bool   saveResultImage = true;      // Save image with detected lines drawn
	bool   printTiming = true;          // Print timing results to console

    // Scan input folder and return all image paths
    std::vector<std::string> scanInputFolder(const std::string& folder) const;
};