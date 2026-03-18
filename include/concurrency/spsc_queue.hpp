#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace hft {

    template <typename T, size_t capacity>
    class SPSCQueue {

        private:
            std::vector<T> buffer_;
            std::atomic<size_t> head_{0};
            std::atomic<size_t> tail_{0};

        public:
            SPSCQueue() : buffer_(capacity + 1) {}  // one slot is wasted
            ~SPSCQueue() = default;

            bool push(const T& item) {
                size_t curr_tail = tail_.load();
                size_t next_tail = (curr_tail + 1) % buffer_.size();

                if(next_tail == head_.load()) return false;

                buffer_[curr_tail] = item;
                tail_.store(next_tail);
                return true;
            }

            bool pop(T& out_item) {
                size_t curr_head = head_.load();

                if(curr_head == tail_.load()) return false;

                out_item = buffer_[curr_head];
                size_t next_head = (curr_head + 1) % buffer_.size();
                head_.store(next_head);
                return true;
            }
    };
}