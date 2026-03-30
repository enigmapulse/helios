#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <atomic>
#include <vector>
#include <sched.h>
#include <csignal>
#include <emmintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h> // For __rdtsc()
#endif

#include "concurrency/spsc_queue.hpp"
#include "engine/order.hpp"
#include "utils/thread_utils.hpp"
#include "utils/telemetry.hpp" 

using namespace hft;

// Global atomic flag for the kill switch
std::atomic<bool> keep_running{true};

void handle_sigint(int sig) {
    keep_running.store(false, std::memory_order_release);
}

// Simulated Network Gateway (The Producer)
void network_gateway_loop(SPSCQueue<Order, 1024>& ring_buffer, uint64_t total_orders, std::atomic<bool>& start_flag) {
    std::cout << "[Gateway] Listening for network traffic...\n";
    
    while (!start_flag.load(std::memory_order_acquire)) _mm_pause();
    
    uint64_t next_order_id = 1;

    while (next_order_id <= total_orders) {
        Side side = ((next_order_id & 1) == 0) ? Side::SELL : Side::BUY;
        u32 price = static_cast<u32>(49990 + (next_order_id % 20));
        Order new_order(next_order_id++, price, 10, side);

        // STAMP 1: Raw hardware clock cycles when the "packet" arrives
        new_order.timestamp = __rdtsc();

        while (!ring_buffer.push(new_order)) {
            if (!keep_running.load(std::memory_order_relaxed)) return;
            _mm_pause();
        }
    }
}

// --- CONSUMER: The Actual Matching Engine ---
void matching_engine_loop(SPSCQueue<Order, 1024>& ring_buffer, OrderBook& book, uint64_t total_orders, std::atomic<bool>& start_flag, std::vector<uint64_t>& latencies) {
    
    // PRE-ALLOCATE: Prevent expensive heap allocations during the hot loop
    latencies.reserve(total_orders);

    while (!start_flag.load(std::memory_order_acquire)) _mm_pause();

    Order incoming;
    uint64_t processed = 0;
    
    while (!ring_buffer.pop(incoming)) {
        if (!keep_running.load(std::memory_order_relaxed)) return;
        _mm_pause();
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    book.add_order(incoming);
    // STAMP 2: Record end-to-end CPU cycles (Gateway -> Engine Execution)
    latencies.push_back(__rdtsc() - incoming.timestamp);
    processed++;

    while (processed < total_orders) {
        if (ring_buffer.pop(incoming)) {
            
            // STAMP: Start the clock right as processing begins
            uint64_t start_tsc = __rdtsc();
            
            book.add_order(incoming);
            
            // Log latency only once every 1024 orders to prevent Cache Eviction!
            if ((processed % 1024) == 0) {
                latencies.push_back(__rdtsc() - start_tsc);
            }
            
            processed++;

        } else {
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
}

int main(int argc, char* argv[]) {
    uint64_t target_orders = 10'000'000; 
    if (argc > 1) {
        try { target_orders = std::stoull(argv[1]); } 
        catch (...) { std::cerr << "[ERROR] Invalid count. Using 10M.\n"; }
    }
    
    // 1. Get the exact CPU frequency before the engine starts
    double tsc_hz = hft::telemetry::estimate_tsc_hz();
    
    std::cout << "=== Starting HFT Matching Engine (Isolated Core Mode) ===\n";
    std::signal(SIGINT, handle_sigint);
    
    std::atomic<bool> start_flag{false};
    SPSCQueue<Order, 1024> engine_queue;
    OrderBook limit_order_book;
    
    // 2. The array that will hold our raw latency data
    std::vector<uint64_t> engine_latencies;

    std::thread gateway_thread(network_gateway_loop, std::ref(engine_queue), target_orders, std::ref(start_flag));
    // Pass the vector to the engine thread
    std::thread engine_thread(matching_engine_loop, std::ref(engine_queue), std::ref(limit_order_book), target_orders, std::ref(start_flag), std::ref(engine_latencies));

    utils::pin_thread_to_core(gateway_thread, 1);
    utils::pin_thread_to_core(engine_thread, 2);
    
    if (!utils::set_realtime_priority(engine_thread)) {
        std::cout << "[INFO] Running without RT priority (Standard OS Scheduling)\n";
    } else {
        std::cout << "[System] Engine thread elevated to SCHED_FIFO Real-Time priority.\n";
    }

    std::cout << "[System] OS configuration complete. Pulling the trigger on " << target_orders << " Orders...\n";

    start_flag.store(true, std::memory_order_release);

    gateway_thread.join();
    engine_thread.join();

    // 3. Print the glorious Histogram!
    hft::telemetry::print_latency_histogram(engine_latencies, tsc_hz);

    return 0;
}