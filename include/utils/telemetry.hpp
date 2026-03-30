#pragma once

#include <vector>
#include <cstdint>

namespace hft {
namespace telemetry {

    /**
     * @brief Calibrates the CPU's Time Stamp Counter (TSC) against the OS clock.
     * @return The estimated CPU frequency in Hz.
     */
    double estimate_tsc_hz();

    /**
     * @brief Sorts raw cycle data, converts to nanoseconds, and prints a histogram.
     * @param cycle_latencies Vector containing the __rdtsc() differences for each order.
     * @param tsc_hz The calibrated CPU frequency.
     */
    void print_latency_histogram(std::vector<uint64_t>& cycle_latencies, double tsc_hz);

} // namespace telemetry
} // namespace hft