#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <type_traits>
#include <utility>

namespace hft {
    //Single Producer Single Consumer lock-free queue.

template <typename T, size_t capacity>
class SPSCQueue {

private:

    // Capacity must be positive
    static_assert(capacity > 0, "capacity must be > 0");

    // Vector pre-allocates storage, so T must be default constructible
    static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

    static constexpr size_t N = capacity + 1;

    // Contiguous ring buffer storage
    std::vector<T> buffer_;

    // Cache-line padded atomics to avoid false sharing.
    struct alignas(64) PaddedAtomic {
        std::atomic<size_t> value{0};
    };

    // index of next element to read (consumer owns writes)
    PaddedAtomic head_;

    // index of next element to write (producer owns writes)
    PaddedAtomic tail_;


public:

    // Pre-allocate buffer once
    SPSCQueue() : buffer_(N) {}
    ~SPSCQueue() = default;

    // Lock-free structures must not be copied or moved
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    bool push(const T& item) noexcept {

        // Only producer writes tail → relaxed is safe
        size_t curr_tail = tail_.value.load(std::memory_order_relaxed);

        // Compute next position in ring buffer
        size_t next_tail = (curr_tail + 1) % N;

        // Acquire synchronizes with consumer's release. Ensures we see the latest head value before checking if queue is full.
        
        if (next_tail == head_.value.load(std::memory_order_acquire)) return false;   // queue full

        // Write element into buffer
        buffer_[curr_tail] = item;

        // Release ensures that the write to buffer_ happens BEFORE the tail update becomes visible to the consumer.
        tail_.value.store(next_tail, std::memory_order_release);

        return true;
    }

    bool pop(T& out_item) noexcept {

        // Only consumer writes head → relaxed is safe
        size_t curr_head = head_.value.load(std::memory_order_relaxed);

        // Acquire synchronizes with producer's release. Guarantees that buffer write by producer is visible before we read it.

        if (curr_head == tail_.value.load(std::memory_order_acquire)) return false;   // queue empty

        // Move element out of buffer
        out_item = std::move(buffer_[curr_head]);

        // Advance head index
        size_t next_head = (curr_head + 1) % N;

        // Release ensures that the read completes before the slot becomes visible as free to the producer.
        head_.value.store(next_head, std::memory_order_release);

        return true;
    }
};

} // namespace hft