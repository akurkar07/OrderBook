#include <cassert>
#include <iostream>

#include "price_level.h"

using namespace orderbook;

void test_price_level_add() {
    PriceLevel level(100.0);
    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);

    level.add_order(order);
    assert(!level.empty());
    assert(level.total_quantity() == 10);
    assert(level.price() == 100.0);
}

void test_price_level_remove() {
    PriceLevel level(100.0);
    Order order1(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order order2(2, Side::Buy, OrderType::Limit, 100.0, 5);

    level.add_order(order1);
    level.add_order(order2);
    assert(level.total_quantity() == 15);

    assert(level.remove_order(1));
    assert(level.total_quantity() == 5);

    assert(level.remove_order(2));
    assert(level.empty());
}

void test_price_level_remove_nonexistent() {
    PriceLevel level(100.0);
    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);
    level.add_order(order);
    assert(!level.remove_order(999));
    assert(level.total_quantity() == 10);
}

void test_price_level_front() {
    PriceLevel level(100.0);
    Order order1(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order order2(2, Side::Buy, OrderType::Limit, 100.0, 5);

    level.add_order(order1);
    level.add_order(order2);

    const PriceLevel& const_level = level;
    assert(const_level.front().id == 1);
    assert(const_level.front().quantity == 10);

    level.reduce_quantity(3);
    assert(level.front().quantity == 7);
    assert(level.total_quantity() == 12);
}

void test_price_level_empty() {
    PriceLevel level(100.0);
    assert(level.empty());

    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);
    level.add_order(order);
    assert(!level.empty());

    level.remove_order(1);
    assert(level.empty());
}

int main() {
    test_price_level_add();
    test_price_level_remove();
    test_price_level_remove_nonexistent();
    test_price_level_front();
    test_price_level_empty();

    std::cout << "All price level tests passed!" << std::endl;
    return 0;
}
