#include "engine/order.hpp"
#include <algorithm>

namespace hft {
    void OrderBook::match_buy(Order& order){
        auto it = asks.begin();
        // Outer loop: Traverse the map using the safe iterator pattern
        while (it != asks.end() && order.quantity > 0 && it->price <= order.price){
            auto& order_list = it->orders;

            // Inner loop: Strict FIFO Queue handling
            while (!order_list.empty() && order.quantity > 0) {
                Order& top = order_list.front();
                u32 traded = std::min(order.quantity, top.quantity);

                order.quantity -= traded;
                top.quantity -= traded;

                if (top.quantity == 0) {
                    order_index.erase(top.id); // Keep index in sync
                    order_list.pop_front();    // Safe, non-iterator deletion
                }
            }  
            
            // Safe vector erasure
            if (order_list.empty()) it = asks.erase(it);
            else ++it;          
        }

        // If not fully filled, add to resting bids and track in index
        if (order.quantity > 0){
            auto it = std::lower_bound(bids.begin(), bids.end(), order.price, 
                [](const PriceLevel& level, u32 p) { return level.price > p; });

            if (it == bids.end() || it->price != order.price) {
                it = bids.insert(it, PriceLevel(order.price));
            }

            it->orders.push_back(order);
            order_index[order.id] = std::prev(it->orders.end());
        }  
    }

    void OrderBook::match_sell(Order& order) {
        auto it = bids.begin();
        // Outer loop: Traverse the map using the safe iterator pattern
        while (it != bids.end() && order.quantity > 0 && it->price >= order.price) {
            auto& order_list = it->orders;

            // Inner loop: Strict FIFO Queue handling
            while (!order_list.empty() && order.quantity > 0) {
                Order& top = order_list.front();
                u32 traded = std::min(order.quantity, top.quantity);

                order.quantity -= traded;
                top.quantity -= traded;

                if (top.quantity == 0) {
                    order_index.erase(top.id);
                    order_list.pop_front();    
                }
            }

            // Safe vector erasure
            if (order_list.empty()) it = bids.erase(it);
            else ++it;
            
        }

        // If not fully filled, add to resting asks and track in index
        if (order.quantity > 0) {
            auto it = std::lower_bound(asks.begin(), asks.end(), order.price, 
                [](const PriceLevel& level, u32 p) { return level.price < p; });

            if (it == asks.end() || it->price != order.price) {
                it = asks.insert(it, PriceLevel(order.price));
            }
            
            it->orders.push_back(order);
            order_index[order.id] = std::prev(it->orders.end());
        }
    }

    void OrderBook::add_order(const Order& in_order) {
        if (in_order.quantity == 0) return;
        if (order_index.count(in_order.id)) return; // Defensive check to prevent duplicate order IDs corrupting the index
        // Explicit mutable copy
        Order order = in_order;
        
        if (order.side == Side::BUY) match_buy(order);
        else match_sell(order);
    }

    void OrderBook::cancel_order(u64 order_id) {
        // Find the order in the lookup table
        // If not found, nothing to cancel
        auto it = order_index.find(order_id);
        
        // Iterator pointing to the order inside the list
        if (it == order_index.end()) return;
        auto list_it = it->second;

        // We need price + side to locate correct book
        u32 price = list_it->price;
        Side side = list_it->side;

        auto remove_from_book = [&](auto& book, auto cmp) {
            // Find price level in bid book safely (Applies to both bid/ask via lambda)
            auto vec_it = std::lower_bound(book.begin(), book.end(), price, cmp);
            if (vec_it != book.end() && vec_it->price == price) {
                vec_it->orders.erase(list_it); // Erase from the list in O(1)
                if (vec_it->orders.empty()) book.erase(vec_it); // Clean up empty level
            }
        };

        if (side == Side::BUY) remove_from_book(bids, [](const PriceLevel& l, u32 p) { return l.price > p; });
        else remove_from_book(asks, [](const PriceLevel& l, u32 p) { return l.price < p; }); // Same logic for ask side

        // Finally, remove the tracking iterator from the index to prevent dangling pointers
        order_index.erase(it);
    }

}