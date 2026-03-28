#include <gtest/gtest.h>
#include "engine/order.hpp"

using namespace hft;

// 1. Basic Initialization Test
TEST(OrderTest, CorrectInit) {
    Order order(1001, 50000, 10, Side::BUY);
    EXPECT_EQ(order.id, 1001);
    EXPECT_EQ(order.price, 50000);
    EXPECT_EQ(order.quantity, 10);
    EXPECT_EQ(order.side, Side::BUY);
}

// 2. Resting Order Test
TEST(OrderBookTest, AddRestingOrder) {
    OrderBook book;
    book.add_order(Order(1, 100, 10, Side::BUY));
    
    EXPECT_TRUE(book.has_bid(100));
    EXPECT_TRUE(book.is_tracked(1));
    EXPECT_FALSE(book.has_ask(100)); // Shouldn't cross over
}

// 3. Exact Match Test
TEST(OrderBookTest, ExactMatch) {
    OrderBook book;
    book.add_order(Order(1, 100, 10, Side::SELL)); // Resting Ask
    EXPECT_TRUE(book.has_ask(100));

    book.add_order(Order(2, 100, 10, Side::BUY));  // Incoming Buy crosses the spread
    
    // Both should be completely gone
    EXPECT_FALSE(book.has_ask(100));
    EXPECT_FALSE(book.has_bid(100));
    EXPECT_FALSE(book.is_tracked(1));
    EXPECT_FALSE(book.is_tracked(2));
}

// 4. Partial Fill Test
TEST(OrderBookTest, PartialFill) {
    OrderBook book;
    book.add_order(Order(1, 100, 10, Side::SELL)); // Resting Ask of 10
    
    book.add_order(Order(2, 100, 15, Side::BUY));  // Incoming Buy of 15
    
    // The Sell should be fully filled and erased
    EXPECT_FALSE(book.has_ask(100));
    EXPECT_FALSE(book.is_tracked(1));
    
    // The Buy should have 5 quantity left and be resting on the book
    EXPECT_TRUE(book.has_bid(100));
    EXPECT_TRUE(book.is_tracked(2));
}

// 5. Price Priority Test (Best price matches first)
TEST(OrderBookTest, PricePriority) {
    OrderBook book;
    // Add two asks at different prices
    book.add_order(Order(1, 105, 10, Side::SELL)); // Worse price
    book.add_order(Order(2, 100, 10, Side::SELL)); // Better price

    // Incoming buy crosses both, but only has quantity for one
    book.add_order(Order(3, 110, 10, Side::BUY)); 

    // The better price (100) should be matched and erased
    EXPECT_FALSE(book.has_ask(100));
    EXPECT_FALSE(book.is_tracked(2));

    // The worse price (105) should still be resting
    EXPECT_TRUE(book.has_ask(105));
    EXPECT_TRUE(book.is_tracked(1));
}

// 6. Cancellation Logic Test
TEST(OrderBookTest, CancelOrder) {
    OrderBook book;
    book.add_order(Order(1, 100, 10, Side::BUY));
    
    EXPECT_TRUE(book.has_bid(100));
    EXPECT_TRUE(book.is_tracked(1));

    // Cancel the order
    book.cancel_order(1);

    // Verify complete removal
    EXPECT_FALSE(book.has_bid(100));
    EXPECT_FALSE(book.is_tracked(1));
}

// 7. Edge Cases Test (Zero Quantity & Duplicates)
TEST(OrderBookTest, EdgeCases) {
    OrderBook book;
    
    // Test Zero Quantity
    book.add_order(Order(1, 100, 0, Side::BUY));
    EXPECT_FALSE(book.has_bid(100));
    EXPECT_FALSE(book.is_tracked(1));

    // Test Duplicate ID protection
    book.add_order(Order(2, 100, 10, Side::BUY)); // First one succeeds
    EXPECT_TRUE(book.is_tracked(2));
    
    book.add_order(Order(2, 105, 20, Side::BUY)); // Second one with same ID should be ignored
    EXPECT_FALSE(book.has_bid(105)); // The 105 price level should never be created
}

TEST(OrderBookTest, VerifiesMemoryPoolUsage) {
    OrderBook book;
    
    size_t initial_blocks = Order::pool.get_stats().available_blocks;
    book.add_order(Order(999, 100, 10, Side::BUY));

    size_t blocks_after_add = Order::pool.get_stats().available_blocks;
    EXPECT_EQ(blocks_after_add, initial_blocks - 1) 
        << "FAIL: The Memory Pool was not used! The OS heap was called.";
    book.cancel_order(999);

    size_t blocks_after_cancel = Order::pool.get_stats().available_blocks;
    EXPECT_EQ(blocks_after_cancel, initial_blocks) 
        << "FAIL: The Memory Pool leaked memory!";
}