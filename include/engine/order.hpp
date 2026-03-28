#pragma once

#include <cstdint>
#include <map>
#include <list>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <new>
#include "memory/memory_pool.hpp"
#include "memory/intrusive_list.hpp"

namespace hft {

    using u8  = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    enum class Side {
        BUY, SELL
    };

    struct Order {
        u64 id;
        u32 price;        // price in ticks instead of floating point
        u32 quantity;
        Side side;

        // Intrusive list pointers
        Order* prev = nullptr;
        Order* next = nullptr;

        Order()=default;

        Order(u64 id, u32 price, u32 quantity, Side side) 
        : id(id), price(price), quantity(quantity), side(side) {}

        // static memory pool shared by all orders
        static MemoryPool<Order, 100000> pool;

        static void* operator new(size_t size) {
            if (size != sizeof(Order)) throw std::bad_alloc();
            return pool.allocate(); // O(1) allocation from arena
        }

        static void operator delete(void* ptr) noexcept {
            pool.deallocate(ptr); // O(1) return to the free-list
        }
    };

    struct PriceLevel {
        u32 price;
        IntrusiveList<Order> orders;

        explicit PriceLevel(u32 p) : price(p) {}
    };

    class OrderBook {

    // list to ensure FIFO ordering for same value of price
    // and ensure deletion from middle too if order cancelled
    private:
        std::vector<PriceLevel> bids; // descending
        std::vector<PriceLevel> asks; // ascending

        // Maps Order ID directly to the raw memory pool pointer
        std::unordered_map<u64, Order*> order_index;

        // helper functions
        void match_buy(Order& order);
        void match_sell(Order& order);

    public:
        OrderBook() {
            bids.reserve(1024);
            asks.reserve(1024);
        }

        ~OrderBook() = default;

        void add_order(const Order& in_order);
        void cancel_order(u64 order_id);
        
        // Helper methods for GoogleTest validation
        bool has_bid(u32 price) const { 
            auto it = std::lower_bound(bids.begin(), bids.end(), price, 
                [](const PriceLevel& level, u32 p) { return level.price > p; }); // > for DESC
            return it != bids.end() && it->price == price; 
        }

        bool has_ask(u32 price) const { 
            auto it = std::lower_bound(asks.begin(), asks.end(), price, 
                [](const PriceLevel& level, u32 p) { return level.price < p; }); // < for ASC
            return it != asks.end() && it->price == price; 
        }

        bool is_tracked(u64 order_id) const { 
            return order_index.find(order_id) != order_index.end(); 
        }
    };
}
