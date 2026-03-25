#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "concurrency/spsc_queue.hpp"
#include "engine/order.hpp"

using namespace hft;

// 1. Basic Sanity Check
TEST(SPSC_Queue_Test, SingleThreadBasic) {
    SPSCQueue<int, 10> q;
    int val = 0;

    EXPECT_FALSE(q.pop(val)); // Should fail, queue is empty

    EXPECT_TRUE(q.push(42));
    EXPECT_TRUE(q.push(99));

    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 42);

    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(val, 99);

    EXPECT_FALSE(q.pop(val)); // Empty again
}

// 2. Boundary Test: Queue Full & Wrap Around
TEST(SPSC_Queue_Test, BoundaryFullEmpty) {
    // Capacity of 3
    SPSCQueue<int, 3> q;
    
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_TRUE(q.push(3));
    
    // 4th push should be rejected instantly
    EXPECT_FALSE(q.push(4)); 

    int val = 0;
    EXPECT_TRUE(q.pop(val)); EXPECT_EQ(val, 1);
    EXPECT_TRUE(q.pop(val)); EXPECT_EQ(val, 2);
    
    // Now push more to test the modulo wrap-around logic
    EXPECT_TRUE(q.push(5));
    EXPECT_TRUE(q.push(6));
    EXPECT_FALSE(q.push(7)); // Full again

    EXPECT_TRUE(q.pop(val)); EXPECT_EQ(val, 3);
    EXPECT_TRUE(q.pop(val)); EXPECT_EQ(val, 5);
    EXPECT_TRUE(q.pop(val)); EXPECT_EQ(val, 6);
    EXPECT_FALSE(q.pop(val)); // Empty
}

// 3. Struct Integration Test: Proves the Default Constructor fix works
TEST(SPSC_Queue_Test, OrderStructSupport) {
    SPSCQueue<Order, 5> q;
    
    Order incoming(1001, 50000, 10, Side::BUY);
    EXPECT_TRUE(q.push(incoming));

    Order popped;
    EXPECT_TRUE(q.pop(popped));
    
    EXPECT_EQ(popped.id, 1001);
    EXPECT_EQ(popped.price, 50000);
    EXPECT_EQ(popped.quantity, 10);
    EXPECT_EQ(popped.side, Side::BUY);
}

// 4. Navneet's Concurrency Stress Test (Scaled up to 100,000 items)
TEST(SPSC_Queue_Test, ConcurrentPushPop100K) {
    SPSCQueue<int, 1024> q;
    const int total_items = 100000;
    
    std::vector<int> consumed;
    consumed.reserve(total_items);

    std::thread producer([&]() {
        for (int i = 0; i < total_items; i++) {
            // Spinlock: Aggressively loop until push succeeds
            while (!q.push(i));
        }
    });

    std::thread consumer([&]() {
        int received = 0;
        int item = 0;
        while (received < total_items) {
            if (q.pop(item)) {
                consumed.push_back(item);
                received++;
            }
        }
    });

    producer.join();
    consumer.join();

    // Verify absolutely no data was dropped or reordered by the CPU
    EXPECT_EQ(consumed.size(), total_items);
    for (int i = 0; i < total_items; i++) {
        EXPECT_EQ(consumed[i], i);
    }
}

TEST(SPSC_Queue_Test, ConcurrentSmallCapacity)
{
    SPSCQueue<int, 4> q;

    const int total = 5000;

    std::vector<int> out;
    out.reserve(total);

    std::thread producer([&]() {
        for (int i = 0; i < total; i++) {
            while (!q.push(i));
        }
    });

    std::thread consumer([&]() {
        int x;
        while (out.size() < total) {
            if (q.pop(x)) {
                out.push_back(x);
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(out.size(), total);

    for (int i = 0; i < total; i++) {
        EXPECT_EQ(out[i], i);
    }
}