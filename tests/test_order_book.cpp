#include <iostream>

#include "types.h"
#include "test_utils.h"

using namespace orderbook;

void test_order_creation() {
    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);
    CHECK(order.id == 1);
    CHECK(order.side == Side::Buy);
    CHECK(order.type == OrderType::Limit);
    CHECK(order.price == 100.0);
    CHECK(order.quantity == 10);
}

void test_default_order_initialisation() {
    Order order;
    CHECK(order.id == 0);
    CHECK(order.side == Side::Buy);
    CHECK(order.type == OrderType::Limit);
    CHECK(order.price == 0.0);
    CHECK(order.quantity == 0);
}

void test_default_fill_initialisation() {
    Fill fill;
    CHECK(fill.id == 0);
    CHECK(fill.buy_order_id == 0);
    CHECK(fill.sell_order_id == 0);
    CHECK(fill.price == 0.0);
    CHECK(fill.quantity == 0);
}

void test_order_type_enum() {
    Order limit(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order market(2, Side::Sell, OrderType::Market, 0.0, 5);

    CHECK(limit.type == OrderType::Limit);
    CHECK(market.type == OrderType::Market);
}

void test_side_enum() {
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order sell(2, Side::Sell, OrderType::Limit, 100.0, 10);

    CHECK(buy.side == Side::Buy);
    CHECK(sell.side == Side::Sell);
}

int main() {
    test_order_creation();
    test_default_order_initialisation();
    test_default_fill_initialisation();
    test_order_type_enum();
    test_side_enum();

    std::cout << "All order tests passed!" << std::endl;
    return 0;
}
