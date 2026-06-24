#include "PipelineConfig.hpp"
#include <filesystem>
#include <algorithm>
#include <cctype>

std::vector<std::string> PipelineConfig::scanInputFolder(const std::string& folder) const {
    std::vector<std::string> paths;

    for (auto& entry : std::filesystem::directory_iterator(folder)) {
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".png" || ext == ".jpg" ||
            ext == ".jpeg" || ext == ".bmp" ||
            ext == ".ppm")
            paths.push_back(entry.path().string());
    }

    std::sort(paths.begin(), paths.end());
    return paths;
}