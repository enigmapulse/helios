#include <gtest/gtest.h>
#include "memory/memory_pool.hpp"
#include "memory/intrusive_list.hpp"

using namespace hft;

// Dummy struct to simulate Issue 8 integration
struct DummyOrder {
    uint64_t id;
    
    // The Intrusive List Pointers to be added in Issue #8
    DummyOrder* prev = nullptr;
    DummyOrder* next = nullptr;
    
    DummyOrder(uint64_t i) : id(i) {}
};

TEST(MemoryArchitectureTest, PoolAllocationAndContiguity) {
    MemoryPool<DummyOrder, 10> pool;
    
    DummyOrder* o1 = new(pool.allocate()) DummyOrder(100);
    DummyOrder* o2 = new(pool.allocate()) DummyOrder(200);
    
    EXPECT_EQ(o1->id, 100);
    EXPECT_EQ(o2->id, 200);
    
    // Prove memory was successfully tracked
    EXPECT_EQ(pool.get_stats().available_blocks, 8);
    
    o1->~DummyOrder(); pool.deallocate(o1);
    o2->~DummyOrder(); pool.deallocate(o2);
    
    EXPECT_EQ(pool.get_stats().available_blocks, 10);
}

TEST(MemoryArchitectureTest, IntrusiveListO1Operations) {
    MemoryPool<DummyOrder, 5> pool;
    IntrusiveList<DummyOrder> list;
    
    auto* o1 = new(pool.allocate()) DummyOrder(1);
    auto* o2 = new(pool.allocate()) DummyOrder(2);
    auto* o3 = new(pool.allocate()) DummyOrder(3);
    
    list.push_back(o1);
    list.push_back(o2);
    list.push_back(o3);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.head()->id, 1);
    EXPECT_EQ(list.tail()->id, 3);
    
    // O(1) Middle Erasure
    list.erase(o2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.head()->next->id, 3);
    EXPECT_EQ(list.tail()->prev->id, 1);
    
    // Cleanup
    while(!list.empty()) {
        auto* popped = list.pop_front();
        popped->~DummyOrder();
        pool.deallocate(popped);
    }
}