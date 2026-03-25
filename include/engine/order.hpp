#pragma once

#include <cstdint>
#include <map>
#include <list>
#include <unordered_map>
#include <algorithm>

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

        Order()=default;

          Order(u64 id, u32 price, u32 quantity, Side side) 
            : id(id), price(price), quantity(quantity), side(side) {}
    };

    class OrderBook {

    // list to ensure FIFO ordering for same value of price
    // and ensure deletion from middle too if order cancelled
    private:
        std::map<u32, std::list<Order>, std::greater<u32>> bids;
        std::map<u32, std::list<Order>, std::less<u32>> asks;

        // lookup Table: maps Order ID -> location in the price list
        std::unordered_map<u64, std::list<Order>::iterator> order_index;

        // helper functions
        void match_buy(Order& order);
        void match_sell(Order& order);

    public:
        OrderBook() = default;
        ~OrderBook() = default;

        void add_order(const Order& in_order);
        void cancel_order(u64 order_id);
        
        // Helper methods for GoogleTest validation
        bool has_bid(u32 price) const { return bids.find(price) != bids.end(); }
        bool has_ask(u32 price) const { return asks.find(price) != asks.end(); }
        bool is_tracked(u64 order_id) const { return order_index.find(order_id) != order_index.end(); }
    };
}
