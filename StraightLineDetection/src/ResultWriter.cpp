#include "ResultWriter.hpp"
#include "PerformanceTimer.hpp"
#include "ImageLoader.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "Pipeline.hpp"
#include <filesystem>

static const double PI = 3.14159265358979323846;

static void drawLineOnImage(Image& img,
    double rho, double theta,
    uint8_t r, uint8_t g, uint8_t b)
{
    int W = img.width;
    int H = img.height;

    double cosT = std::cos(theta);
    double sinT = std::sin(theta);

    // Collect intersection points with the 4 image borders
    std::vector<std::pair<int, int>> pts;

    auto addIfValid = [&](double x, double y) {
        int ix = static_cast<int>(std::round(x));
        int iy = static_cast<int>(std::round(y));
        if (ix >= 0 && ix < W && iy >= 0 && iy < H)
            pts.push_back({ ix, iy });
        };

    // Solve for intersections with each border
    if (std::abs(sinT) > 1e-6) {
        // Top border: y = 0
        addIfValid((rho) / cosT < 1e9 ? (sinT != 0 ? rho / cosT : 0)
            : 0, 0);
        // y=0: x = rho/cosT
        double x_top = (std::abs(cosT) > 1e-6) ? (rho) / cosT : 1e9;
        
        addIfValid(rho / cosT, 0);
    }

    // Find t values where line hits each border
    std::vector<double> tVals;

    if (std::abs(sinT) > 1e-6) {
        // Left border x=0: t = rho*cosT / sinT
        tVals.push_back(rho * cosT / sinT);
        // Right border x=W-1: t = (rho*cosT - (W-1)) / sinT
        tVals.push_back((rho * cosT - (W - 1)) / sinT);
    }
    if (std::abs(cosT) > 1e-6) {
        // Top border y=0: t = -rho*sinT / cosT
        tVals.push_back(-rho * sinT / cosT);
        // Bottom border y=H-1: t = (rho*sinT - (H-1)) / (-cosT)
        tVals.push_back(((H - 1) - rho * sinT) / cosT);
    }

    pts.clear();
    for (double t : tVals) {
        int x = static_cast<int>(std::round(rho * cosT - t * sinT));
        int y = static_cast<int>(std::round(rho * sinT + t * cosT));
        if (x >= 0 && x < W && y >= 0 && y < H)
            pts.push_back({ x, y });
    }

    if (pts.size() < 2) return;

    // Bresenham's line algorithm between first and last valid border point
    int x0 = pts.front().first, y0 = pts.front().second;
    int x1 = pts.back().first, y1 = pts.back().second;

    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (x0 >= 0 && x0 < W && y0 >= 0 && y0 < H) {
            img.at(y0, x0, 0) = r;
            img.at(y0, x0, 1) = g;
            img.at(y0, x0, 2) = b;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

Image ResultWriter::drawLines(const Image& original,
    const std::vector<Line>& lines,
    uint8_t r, uint8_t g, uint8_t b)
{
    // Work on a copy so original is preserved
    Image result = original;

    if (result.channels == 1) {
        Image rgb;
        rgb.width = result.width;
        rgb.height = result.height;
        rgb.channels = 3;
        rgb.data.resize(result.width * result.height * 3);
        for (int i = 0; i < result.width * result.height; ++i) {
            rgb.data[i * 3 + 0] = result.data[i];
            rgb.data[i * 3 + 1] = result.data[i];
            rgb.data[i * 3 + 2] = result.data[i];
        }
        result = rgb;
    }

    for (const auto& line : lines)
        drawLineOnImage(result, line.rho, line.theta, r, g, b);

    std::cout << "[ResultWriter] Drew " << lines.size() << " lines.\n";
    return result;
}

Image ResultWriter::visualizeAccumulator(const HoughTransform::Accumulator& acc) {
    if (acc.data.empty()) return {};

    // Find max value for normalization
    int maxVal = *std::max_element(acc.data.begin(), acc.data.end());
    if (maxVal == 0) return {};

    Image viz;
    viz.width = acc.thetaCount;
    viz.height = acc.rhoCount;
    viz.channels = 1;
    viz.data.resize(acc.thetaCount * acc.rhoCount);

    for (int i = 0; i < acc.rhoCount * acc.thetaCount; ++i) {
        viz.data[i] = static_cast<uint8_t>(
            255.0 * acc.data[i] / maxVal);
    }

    return viz;
}

void ResultWriter::saveReport(const std::string& path,
    const std::vector<Line>& lines,
    const PerformanceTimer& timer,
    const std::string& imageName,
    double serialTotalMs)
{
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[ResultWriter] Cannot open report file: " << path << "\n";
        return;
    }

    f << "================================================\n";
    f << "  Hough Transform — Results Report\n";
    f << "================================================\n";
    f << "Image: " << imageName << "\n\n";

    f << "--- Detected Lines (" << lines.size() << ") ---\n";
    for (size_t i = 0; i < lines.size(); ++i) {
        f << std::left << std::setw(4) << (i + 1)
            << "rho=" << std::setw(10) << std::fixed << std::setprecision(1) << lines[i].rho
            << "theta=" << std::setw(10) << std::fixed << std::setprecision(4) << lines[i].theta
            << "votes=" << lines[i].votes << "\n";
    }

    f << "\n--- Timing (ms) ---\n";
    const std::vector<std::string> phases = {
        "1. Load image",
        "2. Grayscale conversion",
        "3. Edge detection (Sobel)",
        "4. Hough transform",
        "5. Line detection"
    };
    double total = 0.0;
    for (auto& p : phases) {
        double ms = timer.get(p);
        total += ms;
        f << std::left << std::setw(30) << p
            << std::right << std::setw(8) << std::fixed
            << std::setprecision(2) << ms << " ms\n";
    }
    f << std::string(40, '-') << "\n";
    f << std::left << std::setw(30) << "Total"
        << std::right << std::setw(8) << std::fixed
        << std::setprecision(2) << total << " ms\n";

    if (serialTotalMs > 0.0) {
        f << "\n--- Speedup vs serial ---\n";
        f << "Serial total : " << std::fixed << std::setprecision(2)
            << serialTotalMs << " ms\n";
        f << "Parallel total: " << std::fixed << std::setprecision(2)
            << total << " ms\n";
        f << "Speedup      : " << std::fixed << std::setprecision(2)
            << (serialTotalMs / total) << "x\n";
    }

    f << "\n================================================\n";
    std::cout << "[ResultWriter] Report saved: " << path << "\n";
}

void ResultWriter::saveAll(const PipelineData& data,
    const PerformanceTimer& timer,
    const std::string& outputRoot,
    double serialTotalMs)
{
    std::string stem = std::filesystem::path(data.imagePath).stem().string();
    std::string baseName = std::filesystem::path(data.imagePath).filename().string();
    std::string folder = outputRoot + "/" + stem;
    std::filesystem::create_directories(folder);

    ImageLoader::save(data.getGray(), folder + "/gray.png");
    ImageLoader::save(data.getEdges(), folder + "/edges.png");

    Image result = ResultWriter::drawLines(data.getOriginal(), data.getLines());
    ImageLoader::save(result, folder + "/result.png");

    Image accViz = ResultWriter::visualizeAccumulator(data.getAccumulator());
    ImageLoader::save(accViz, folder + "/accumulator.png");

    ResultWriter::saveReport(folder + "/report.txt",
        data.getLines(), timer, baseName, serialTotalMs);
}