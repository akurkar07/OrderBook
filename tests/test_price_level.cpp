#include <iostream>
#include <limits>
#include <stdexcept>

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

void test_price_level_quantity_overflow_is_rejected() {
    PriceLevel level(100.0);
    const Quantity max_quantity = std::numeric_limits<Quantity>::max();
    level.add_order(Order(1, Side::Buy, OrderType::Limit, 100.0, max_quantity - 5));

    bool threw = false;
    try {
        level.add_order(Order(2, Side::Buy, OrderType::Limit, 100.0, 10));
    } catch (const std::overflow_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(level.total_quantity() == max_quantity - 5);
    CHECK(level.front().id == 1);
    CHECK(level.front().quantity == max_quantity - 5);
    CHECK(!level.remove_order(2));
}

int main() {
    test_price_level_add();
    test_price_level_remove();
    test_price_level_remove_nonexistent();
    test_price_level_front();
    test_price_level_empty();
    test_price_level_quantity_overflow_is_rejected();

    std::cout << "All price level tests passed!" << std::endl;
    return 0;
}
