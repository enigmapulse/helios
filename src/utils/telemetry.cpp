#include "utils/telemetry.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace hft {
namespace telemetry {

    double estimate_tsc_hz() {
        std::cout << "[Telemetry] Calibrating CPU Time Stamp Counter (TSC)...\n";
        
        // Warm up the CPU
        auto start_chrono = std::chrono::high_resolution_clock::now();
        uint64_t start_tsc = __rdtsc();
        
        // Sleep for exactly 100 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        uint64_t end_tsc = __rdtsc();
        auto end_chrono = std::chrono::high_resolution_clock::now();
        
        // Calculate exact time elapsed in seconds
        std::chrono::duration<double> elapsed_seconds = end_chrono - start_chrono;
        
        // Cycles per second (Hz) = Total Cycles / Total Seconds
        double tsc_hz = static_cast<double>(end_tsc - start_tsc) / elapsed_seconds.count();
        
        std::cout << "[Telemetry] CPU Frequency Calibrated: " 
                  << std::fixed << std::setprecision(2) << (tsc_hz / 1'000'000'000.0) << " GHz\n";
                  
        return tsc_hz;
    }

    void print_latency_histogram(std::vector<uint64_t>& cycle_latencies, double tsc_hz) {
        if (cycle_latencies.empty()) {
            std::cerr << "[Telemetry] No latency data to process.\n";
            return;
        }

        std::cout << "\n=== Engine Latency Histogram (Nanoseconds) ===\n";
        std::cout << "Sorting " << cycle_latencies.size() << " data points...\n";

        // Sort the data to find percentiles
        std::sort(cycle_latencies.begin(), cycle_latencies.end());

        // Lambda to convert cycles to nanoseconds
        auto cycles_to_ns = [tsc_hz](uint64_t cycles) -> double {
            return (static_cast<double>(cycles) / tsc_hz) * 1'000'000'000.0;
        };

        size_t n = cycle_latencies.size();
        
        // Calculate exact indices for percentiles
        size_t idx_50 = static_cast<size_t>(n * 0.50);
        size_t idx_90 = static_cast<size_t>(n * 0.90);
        size_t idx_99 = static_cast<size_t>(n * 0.99);
        size_t idx_999 = static_cast<size_t>(n * 0.999);
        size_t idx_max = n - 1;

        std::cout << "----------------------------------------------\n";
        std::cout << " Metric    | Cycles       | Time (ns)         \n";
        std::cout << "----------------------------------------------\n";
        
        auto print_row = [&](const char* label, size_t index) {
            uint64_t cycles = cycle_latencies[index];
            std::cout << " " << std::left << std::setw(9) << label 
                      << " | " << std::left << std::setw(12) << cycles 
                      << " | " << std::fixed << std::setprecision(2) << cycles_to_ns(cycles) << " ns\n";
        };

        print_row("Min", 0);
        print_row("p50 (Med)", idx_50);
        print_row("p90", idx_90);
        print_row("p99", idx_99);
        print_row("p99.9", idx_999);
        print_row("Max", idx_max);
        std::cout << "----------------------------------------------\n";
    }

} // namespace telemetry
} // namespace hft