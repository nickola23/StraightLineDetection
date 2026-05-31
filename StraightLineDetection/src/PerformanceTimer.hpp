#pragma once
#include <chrono>
#include <string>
#include <unordered_map>
#include <iostream>
#include <iomanip>

class PerformanceTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::duration<double, std::milli>;

    void start(const std::string& phase) {
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
        std::cout << "\n=== Timing results ===\n";
        double total = 0.0;
        for (auto& [phase, ms] : durations_) {
            std::cout << std::left << std::setw(28) << phase
                << std::right << std::setw(8) << std::fixed
                << std::setprecision(2) << ms << " ms\n";
            total += ms;
        }
        std::cout << std::string(38, '-') << "\n";
        std::cout << std::left << std::setw(28) << "Total"
            << std::right << std::setw(8) << std::fixed
            << std::setprecision(2) << total << " ms\n";
    }

private:
    std::unordered_map<std::string, Clock::time_point> starts_;
    std::unordered_map<std::string, double>            durations_;
};