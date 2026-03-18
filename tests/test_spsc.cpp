#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "concurrency/spsc_queue.hpp"

using namespace hft;

TEST(SPSC_Queue_Test, ConcurrentPushPop10K) {

    SPSCQueue <int, 1024> q;

    const int total_items = 10000;
    std::vector<int> consumed;
    consumed.reserve(total_items);

    std::thread producer(
        [&]() {
            for (int i = 0; i < total_items; i++) {
                while(!q.push(i));
            }
        }
    );

    std::thread consumer(
        [&]() {
            int received = 0;
            int item = 0;
            while(received < total_items) {
                if(q.pop(item)) {
                    consumed.push_back(item);
                    received++;
                }
            }
        }
    );

    producer.join();
    consumer.join();

    EXPECT_EQ(consumed.size(), total_items);
    for (int i = 0; i < total_items; i++) {
        EXPECT_EQ(consumed[i], i);
    }
}