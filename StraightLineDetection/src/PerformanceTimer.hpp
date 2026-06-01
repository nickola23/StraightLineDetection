#pragma once
#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

class PerformanceTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::duration<double, std::milli>;

    void start(const std::string& phase) {
        if (order_.find(phase) == order_.end())
            order_[phase] = static_cast<int>(order_.size());
        starts_[phase] = Clock::now();
    }

    void stop(const std::string& phase) {
        auto end = Clock::now();
        durations_[phase] = Duration(end - starts_[phase]).count();
    }

    double get(const std::string& phase) const {
        auto it = durations_.find(phase);
        return (it != durations_.end()) ? it->second : 0.0;
    }

    void printAll() const {
        std::vector<std::pair<int, std::string>> sorted;
        for (auto& [phase, idx] : order_)
            sorted.push_back({ idx, phase });
        std::sort(sorted.begin(), sorted.end());

        std::cout << "\n=== Timing results ===\n";
        double total = 0.0;
        for (auto& [idx, phase] : sorted) {
            double ms = get(phase);
            std::cout << std::left << std::setw(30) << phase
                << std::right << std::setw(8) << std::fixed
                << std::setprecision(2) << ms << " ms\n";
            total += ms;
        }
        std::cout << std::string(40, '-') << "\n";
        std::cout << std::left << std::setw(30) << "Total"
            << std::right << std::setw(8) << std::fixed
            << std::setprecision(2) << total << " ms\n";
    }

private:
    std::map<std::string, Clock::time_point> starts_;
    std::map<std::string, double>            durations_;
    std::map<std::string, int>               order_;
};