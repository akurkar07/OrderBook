#include <iostream>

#include "price_level.h"
#include "test_utils.h"

using namespace orderbook;

void test_price_level_add() {
    PriceLevel level(100.0);
    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);

    level.add_order(order);
    CHECK(!level.empty());
    CHECK(level.total_quantity() == 10);
    CHECK(level.price() == 100.0);
}

void test_price_level_remove() {
    PriceLevel level(100.0);
    Order order1(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order order2(2, Side::Buy, OrderType::Limit, 100.0, 5);

    level.add_order(order1);
    level.add_order(order2);
    CHECK(level.total_quantity() == 15);

    CHECK(level.remove_order(1));
    CHECK(level.total_quantity() == 5);

    CHECK(level.remove_order(2));
    CHECK(level.empty());
}

void test_price_level_remove_nonexistent() {
    PriceLevel level(100.0);
    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);
    level.add_order(order);
    CHECK(!level.remove_order(999));
    CHECK(level.total_quantity() == 10);
}

void test_price_level_front() {
    PriceLevel level(100.0);
    Order order1(1, Side::Buy, OrderType::Limit, 100.0, 10);
    Order order2(2, Side::Buy, OrderType::Limit, 100.0, 5);

    level.add_order(order1);
    level.add_order(order2);

    const PriceLevel& const_level = level;
    CHECK(const_level.front().id == 1);
    CHECK(const_level.front().quantity == 10);

    level.reduce_quantity(3);
    CHECK(level.front().quantity == 7);
    CHECK(level.total_quantity() == 12);
}

void test_price_level_empty() {
    PriceLevel level(100.0);
    CHECK(level.empty());

    Order order(1, Side::Buy, OrderType::Limit, 100.0, 10);
    level.add_order(order);
    CHECK(!level.empty());

    level.remove_order(1);
    CHECK(level.empty());
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
