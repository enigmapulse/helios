#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <atomic>
#include <sched.h>
#include "utils/timer.hpp"
#include "concurrency/spsc_queue.hpp"
#include "engine/order.hpp"
#include "utils/thread_utils.hpp"
#include <csignal>
#include <emmintrin.h>

using namespace hft;


// Global atomic flag for the kill switch
std::atomic<bool> keep_running{true};

// Signal handler to catch Ctrl+C
void handle_sigint(int sig) {
    keep_running.store(false, std::memory_order_release);
}

// Simulated Network Gateway (The Producer)
void network_gateway_loop(SPSCQueue<Order, 1024>& ring_buffer, uint64_t total_orders, std::atomic<bool>& start_flag) {
    std::cout << "[Gateway] Listening for network traffic...\n";
    
    while (!start_flag.load(std::memory_order_acquire)) _mm_pause();
    
    uint64_t next_order_id = 1;

    while (next_order_id <= total_orders) {
        
        // Using the bitwise micro-optimization
        Side side = ((next_order_id & 1) == 0) ? Side::SELL : Side::BUY;
        u32 price = static_cast<u32>(49990 + (next_order_id % 20));
        Order new_order(next_order_id++, price, 10, side);

        new_order.timestamp = utils::get_nanoseconds();

        while (!ring_buffer.push(new_order)) {
            // COLD PATH: Only check the kill switch if the queue is full
            if (!keep_running.load(std::memory_order_relaxed)) return;
            _mm_pause();
        }
    }
}

// --- CONSUMER: The Actual Matching Engine ---
void matching_engine_loop(SPSCQueue<Order, 1024>& ring_buffer, OrderBook& book, uint64_t total_orders, std::atomic<bool>& start_flag) {
    while (!start_flag.load(std::memory_order_acquire)) _mm_pause();

    Order incoming;
    uint64_t processed = 0;
    
    while (!ring_buffer.pop(incoming)) {
        if (!keep_running.load(std::memory_order_relaxed)) return;
        _mm_pause();
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    book.add_order(incoming);
    processed++;

    uint64_t final_order_latency = 0;

    while (processed < total_orders) {
        // HOT PATH: If pop succeeds, process immediately.
        if (ring_buffer.pop(incoming)) {

            uint64_t start_process_ts = 0;
            if (processed == total_orders - 1) {
                start_process_ts = utils::get_nanoseconds();
            }

            book.add_order(incoming);
            processed++;

            if (processed == total_orders) {
                uint64_t end_ts = utils::get_nanoseconds();
                final_order_latency = end_ts - start_process_ts;
            }
        } else {
            // COLD PATH: Only check the kill switch if the queue is empty
            if (!keep_running.load(std::memory_order_relaxed)) break; 
            _mm_pause(); 
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    if (duration_ns == 0) duration_ns = 1; 

    double ops_per_sec = (static_cast<double>(processed) / duration_ns) * 1'000'000'000.0;
    
    std::cout << "\n[Engine] Processed " << processed << " orders in " 
              << (duration_ns / 1'000'000.0) << " ms (" << duration_ns << " ns).\n";
    std::cout << "[Engine] Throughput: " << static_cast<uint64_t>(ops_per_sec) << " ops/sec.\n";
    std::cout << "[Engine] Final Engine Core: " << sched_getcpu() << "\n";

    std::cout << "[Engine] End-to-End Latency (Last Order): " << final_order_latency << " ns.\n";
    std::cout << "[Engine] Final Engine Core: " << sched_getcpu() << "\n";
}

int main(int argc, char* argv[]) {
    uint64_t target_orders = 10'000'000; 
    if (argc > 1) {
        try { target_orders = std::stoull(argv[1]); } 
        catch (...) { std::cerr << "[ERROR] Invalid count. Using 10M.\n"; }
    }

    std::cout << "=== Starting HFT Matching Engine (Isolated Core Mode) ===\n";
    std::signal(SIGINT, handle_sigint);
    // The Starting Flag
    std::atomic<bool> start_flag{false};

    SPSCQueue<Order, 1024> engine_queue;
    OrderBook limit_order_book;

    std::thread gateway_thread(network_gateway_loop, std::ref(engine_queue), target_orders, std::ref(start_flag));
    std::thread engine_thread(matching_engine_loop, std::ref(engine_queue), std::ref(limit_order_book), target_orders, std::ref(start_flag));

    // Core 0 reserved for OS, 1=producer, 2=consumer
    utils::pin_thread_to_core(gateway_thread, 1);
    utils::pin_thread_to_core(engine_thread, 2);
    
    //REALTIME FALLBACK LOGIC
    if (!utils::set_realtime_priority(engine_thread)) {
        std::cout << "[INFO] Running without RT priority (Standard OS Scheduling)\n";
        std::cout << "[INFO] Tip: Run with 'sudo' to achieve true SCHED_FIFO isolation.\n";
    } else {
        std::cout << "[System] Engine thread elevated to SCHED_FIFO Real-Time priority.\n";
    }

    std::cout << "[System] OS configuration complete. Pulling the trigger on " << target_orders << " Orders...\n";

    //Release both threads simultaneously
    start_flag.store(true, std::memory_order_release);

    gateway_thread.join();
    engine_thread.join();

    return 0;
}