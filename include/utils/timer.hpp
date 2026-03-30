#pragma once

#include <cstdint>
#include <chrono>

namespace hft {
namespace utils {

    /**
     * @brief Gets the current time in nanoseconds.
     * Uses the high-resolution monotonic clock to ensure time never goes backwards.
     */
    inline uint64_t get_nanoseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }

} // namespace utils
} // namespace hft