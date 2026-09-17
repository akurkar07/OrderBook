#include <iostream>
#include <limits>
#include <stdexcept>

#include "order_book.h"
#include "price_level.h"
#include "test_utils.h"

using namespace orderbook;

void test_order_book_rejects_level_overflow_without_throwing() {
    OrderBook book;
    const Quantity max_quantity = std::numeric_limits<Quantity>::max();

    auto first_fills = book.place_limit_order(
        Order(1, Side::Buy, OrderType::Limit, 100.0, max_quantity - 5));
    CHECK(first_fills.empty());

    bool threw = false;
    Fills overflow_fills;
    try {
        overflow_fills = book.place_limit_order(
            Order(2, Side::Buy, OrderType::Limit, 100.0, 10));
    } catch (...) {
        threw = true;
    }

    CHECK(!threw);
    CHECK(overflow_fills.empty());
    CHECK(!book.cancel_order(2));
    CHECK(book.cancel_order(1));
    CHECK(book.empty());
}

void test_order_book_accepts_exact_level_capacity() {
    OrderBook book;
    const Quantity max_quantity = std::numeric_limits<Quantity>::max();

    book.place_limit_order(
        Order(1, Side::Sell, OrderType::Limit, 100.0, max_quantity - 5));
    auto fills = book.place_limit_order(
        Order(2, Side::Sell, OrderType::Limit, 100.0, 5));

    CHECK(fills.empty());
    CHECK(book.cancel_order(2));
    CHECK(book.cancel_order(1));
    CHECK(book.empty());
}

void test_invalid_price_level_prices_are_rejected() {
    for (Price price : {0.0,
                        -1.0,
                        std::numeric_limits<Price>::infinity(),
                        std::numeric_limits<Price>::quiet_NaN()}) {
        bool threw = false;
        try {
            PriceLevel level(price);
            (void)level;
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);
    }
}

void test_price_level_rejects_invalid_orders() {
    const Side invalid_side = static_cast<Side>(255);

    {
        PriceLevel level(100.0);
        bool threw = false;
        try {
            level.add_order(Order(1, Side::Buy, OrderType::Limit, 100.0, 0));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);
        CHECK(level.empty());
    }

    {
        PriceLevel level(100.0);
        bool threw = false;
        try {
            level.add_order(Order(1, Side::Buy, OrderType::Market, 100.0, 5));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);
        CHECK(level.empty());
    }

    {
        PriceLevel level(100.0);
        bool threw = false;
        try {
            level.add_order(Order(1, Side::Buy, OrderType::Limit, 101.0, 5));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);
        CHECK(level.empty());
    }

    {
        PriceLevel level(100.0);
        bool threw = false;
        try {
            level.add_order(Order(1, invalid_side, OrderType::Limit, 100.0, 5));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);
        CHECK(level.empty());
    }
}

void test_empty_price_level_access_is_defined() {
    PriceLevel level(100.0);

    bool front_threw = false;
    try {
        (void)level.front();
    } catch (const std::out_of_range&) {
        front_threw = true;
    }
    CHECK(front_threw);

    bool reduce_threw = false;
    try {
        level.reduce_quantity(1);
    } catch (const std::out_of_range&) {
        reduce_threw = true;
    }
    CHECK(reduce_threw);
    CHECK(level.empty());
    CHECK(level.total_quantity() == 0);
}

void test_excessive_reduction_is_rejected_without_mutation() {
    PriceLevel level(100.0);
    level.add_order(Order(1, Side::Buy, OrderType::Limit, 100.0, 5));

    bool threw = false;
    try {
        level.reduce_quantity(6);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(level.front().id == 1);
    CHECK(level.front().quantity == 5);
    CHECK(level.total_quantity() == 5);
}

int main() {
    test_order_book_rejects_level_overflow_without_throwing();
    test_order_book_accepts_exact_level_capacity();
    test_invalid_price_level_prices_are_rejected();
    test_price_level_rejects_invalid_orders();
    test_empty_price_level_access_is_defined();
    test_excessive_reduction_is_rejected_without_mutation();

    std::cout << "All robustness tests passed!" << std::endl;
    return 0;
}
