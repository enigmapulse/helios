#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "concurrency/spsc_queue.hpp"

using namespace hft;

int main() {
    const int total_items = 5'000'000; 
    SPSCQueue<int, 1024> q;

    std::atomic<bool> start_flag{false};

    std::thread producer([&]() {
        while (!start_flag.load(std::memory_order_acquire)); 
        
        for (int i = 0; i < total_items; i++) {
            while (!q.push(i));
        }
    });

    std::thread consumer([&]() {
        int received = 0;
        int item = 0;
        uint64_t checksum = 0; 

        while (!start_flag.load(std::memory_order_acquire)); 

        while (received < total_items) {
            if (q.pop(item)) {
                checksum += item; 
                received++;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // START THE STOPWATCH 
    auto start_time = std::chrono::high_resolution_clock::now();
    start_flag.store(true, std::memory_order_release);

    // Wait for the sprint to finish
    producer.join();
    consumer.join();
    
    // STOP THE STOPWATCH
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculation
    std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;
    double ms = duration_ms.count();
    double messages_per_sec = (total_items / (ms / 1000.0));

    std::cout << "=== Lock-Free Queue Benchmark ===" << std::endl;
    std::cout << "Items Processed : " << total_items << std::endl;
    std::cout << "Total Time      : " << ms << " ms" << std::endl;
    std::cout << "Throughput      : " << messages_per_sec / 1'000'000.0 << " Million ops/sec" << std::endl;

    return 0;
}