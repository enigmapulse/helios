#include <gtest/gtest.h>
#include "engine/order.hpp"

TEST(OrderTest, CorrectInit) {
    hft::Order order(1001, 50000, 10, hft::Side::BUY);

    EXPECT_EQ(order.id, 1001);
    EXPECT_EQ(order.price, 50000);
    EXPECT_EQ(order.quantity, 10);
    EXPECT_EQ(order.side, hft::Side::BUY);
}