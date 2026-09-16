#include <cassert>
#include <iostream>

#include "order.h"

using namespace orderbook;

void test_order_creation() {
    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);
    assert(order.id == 1);
    assert(order.side == Side::Buy);
    assert(order.type == OrderType::Limit);
    assert(order.price == 100.0);
    assert(order.quantity == 10);
}

void test_order_type_enum() {
    Order limit(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order market(2, Side::Sell, OrderType::Market, 0.0, 5);

    assert(limit.type == OrderType::Limit);
    assert(market.type == OrderType::Market);
}

void test_side_enum() {
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order sell(2, Side::Sell, OrderType::Limit, 100.0, 10);

    assert(buy.side == Side::Buy);
    assert(sell.side == Side::Sell);
}

int main() {
    test_order_creation();
    test_order_type_enum();
    test_side_enum();

    std::cout << "All order tests passed!" << std::endl;
    return 0;
}
