#pragma once

#include <cstdint>
#include <map>
#include <list>
#include <unordered_map>

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

          Order(u64 id, u32 price, u32 quantity, Side side) 
            : id(id), price(price), quantity(quantity), side(side) {}
    };

    class OrderBook {

        // list to ensure FIFO ordering for same value of price
        // and ensure deletion from middle too if order cancelled
        private:
            std::map<u32, std::list<Order>, std::greater<u32>> bids;
            std::map<u32, std::list<Order>, std::less<u32>> asks;

            // lookup Table: maps Order ID -> location in the list
            std::unordered_map<u64, std::list<Order>::iterator> order_index;

        public:
            OrderBook() = default;
            ~OrderBook() = default;

            void add_order(const Order& order) {
                // TODO : implement matching and resting and cancellation logic
            }
    };
}
