#include <iostream>
#include <limits>

#include "order_book.h"
#include "test_utils.h"

using namespace orderbook;

void test_limit_order_no_match() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    auto fills = book.place_limit_order(buy);

    CHECK(fills.empty());
    CHECK(!book.empty());

    Order sell(2, Side::Sell, OrderType::Limit, 101.0, 5);
    fills = book.place_limit_order(sell);
    CHECK(fills.empty());
}

void test_limit_order_full_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    Order sell(2, Side::Sell, OrderType::Limit, 100.0, 10);
    auto fills = book.place_limit_order(sell);

    CHECK(fills.size() == 1);
    CHECK(fills[0].quantity == 10);
    CHECK(fills[0].price == 100.0);
    CHECK(fills[0].buy_order_id == 1);
    CHECK(fills[0].sell_order_id == 2);
    CHECK(book.empty());
}

void test_limit_order_partial_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    Order sell(2, Side::Sell, OrderType::Limit, 100.0, 3);
    auto fills = book.place_limit_order(sell);

    CHECK(fills.size() == 1);
    CHECK(fills[0].quantity == 3);
    CHECK(!book.empty());

    Order sell2(3, Side::Sell, OrderType::Limit, 100.0, 7);
    fills = book.place_limit_order(sell2);
    CHECK(fills.size() == 1);
    CHECK(fills[0].quantity == 7);
    CHECK(book.empty());
}

void test_limit_partial_fill_rests_remainder() {
    {
        OrderBook book;
        book.place_limit_order(Order(1, Side::Buy, OrderType::Limit, 100.0, 5));

        auto fills = book.place_limit_order(Order(2, Side::Sell, OrderType::Limit, 100.0, 8));
        CHECK(fills.size() == 1);
        CHECK(fills[0].quantity == 5);
        CHECK(!book.empty());

        auto remainder_fill = book.place_market_order(Order(3, Side::Buy, OrderType::Market, 0.0, 3));
        CHECK(remainder_fill.size() == 1);
        CHECK(remainder_fill[0].sell_order_id == 2);
        CHECK(remainder_fill[0].quantity == 3);
        CHECK(remainder_fill[0].price == 100.0);
        CHECK(book.empty());
    }

    {
        OrderBook book;
        book.place_limit_order(Order(1, Side::Sell, OrderType::Limit, 100.0, 5));

        auto fills = book.place_limit_order(Order(2, Side::Buy, OrderType::Limit, 100.0, 8));
        CHECK(fills.size() == 1);
        CHECK(fills[0].quantity == 5);
        CHECK(!book.empty());

        auto remainder_fill = book.place_market_order(Order(3, Side::Sell, OrderType::Market, 0.0, 3));
        CHECK(remainder_fill.size() == 1);
        CHECK(remainder_fill[0].buy_order_id == 2);
        CHECK(remainder_fill[0].quantity == 3);
        CHECK(remainder_fill[0].price == 100.0);
        CHECK(book.empty());
    }
}

void test_multi_level_fill() {
    OrderBook book;
    Order buy1(1, Side::Buy, OrderType::Limit, 100.0, 5);
    Order buy2(2, Side::Buy, OrderType::Limit, 99.0, 10);
    Order buy3(3, Side::Buy, OrderType::Limit, 98.0, 15);
    book.place_limit_order(buy1);
    book.place_limit_order(buy2);
    book.place_limit_order(buy3);

    Order sell(4, Side::Sell, OrderType::Limit, 98.0, 20);
    auto fills = book.place_limit_order(sell);

    CHECK(fills.size() == 3);
    CHECK(fills[0].quantity == 5);
    CHECK(fills[0].price == 100.0);
    CHECK(fills[1].quantity == 10);
    CHECK(fills[1].price == 99.0);
    CHECK(fills[2].quantity == 5);
    CHECK(fills[2].price == 98.0);
    CHECK(!book.empty());
}

void test_market_order_full_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    Order market_sell(2, Side::Sell, OrderType::Market, 0.0, 10);
    auto fills = book.place_market_order(market_sell);

    CHECK(fills.size() == 1);
    CHECK(fills[0].quantity == 10);
    CHECK(fills[0].price == 100.0);
    CHECK(book.empty());
}

void test_market_order_partial_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 5);
    book.place_limit_order(buy);

    Order market_sell(2, Side::Sell, OrderType::Market, 0.0, 10);
    auto fills = book.place_market_order(market_sell);

    CHECK(fills.size() == 1);
    CHECK(fills[0].quantity == 5);
    CHECK(book.empty());
}

void test_cancel_order() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    CHECK(book.cancel_order(1));
    CHECK(book.empty());
}

void test_cancel_nonexistent_order() {
    OrderBook book;
    CHECK(!book.cancel_order(999));
}

void test_cancel_partial_fill_level() {
    OrderBook book;
    Order buy1(1, Side::Buy, OrderType::Limit, 100.0, 5);
    Order buy2(2, Side::Buy, OrderType::Limit, 100.0, 5);
    book.place_limit_order(buy1);
    book.place_limit_order(buy2);

    CHECK(book.cancel_order(1));
    CHECK(!book.empty());

    CHECK(book.cancel_order(2));
    CHECK(book.empty());
}

void test_partial_fill_with_remainder() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 5);
    book.place_limit_order(buy);

    Order market_sell(2, Side::Sell, OrderType::Market, 0.0, 10);
    auto fills = book.place_market_order(market_sell);

    CHECK(fills.size() == 1);
    CHECK(fills[0].quantity == 5);
    CHECK(book.empty());
}

void test_best_price_priority() {
    OrderBook book;
    Order sell1(1, Side::Sell, OrderType::Limit, 101.0, 5);
    Order sell2(2, Side::Sell, OrderType::Limit, 100.0, 5);
    Order sell3(3, Side::Sell, OrderType::Limit, 102.0, 5);
    book.place_limit_order(sell1);
    book.place_limit_order(sell2);
    book.place_limit_order(sell3);

    Order buy(4, Side::Buy, OrderType::Limit, 101.0, 5);
    auto fills = book.place_limit_order(buy);

    CHECK(fills.size() == 1);
    CHECK(fills[0].price == 100.0);
    CHECK(fills[0].quantity == 5);
}

void test_time_priority_at_same_price() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Sell, OrderType::Limit, 100.0, 3));
    book.place_limit_order(Order(2, Side::Sell, OrderType::Limit, 100.0, 4));

    auto fills = book.place_market_order(Order(3, Side::Buy, OrderType::Market, 0.0, 5));

    CHECK(fills.size() == 2);
    CHECK(fills[0].sell_order_id == 1);
    CHECK(fills[0].quantity == 3);
    CHECK(fills[1].sell_order_id == 2);
    CHECK(fills[1].quantity == 2);
}

void test_duplicate_active_order_id_is_rejected() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Buy, OrderType::Limit, 100.0, 5));
    auto duplicate_fills = book.place_limit_order(Order(1, Side::Buy, OrderType::Limit, 99.0, 5));

    CHECK(duplicate_fills.empty());
    CHECK(book.cancel_order(1));

    auto fills = book.place_market_order(Order(2, Side::Sell, OrderType::Market, 0.0, 10));
    CHECK(fills.empty());
    CHECK(book.empty());
}

void test_market_entrypoint_uses_market_semantics() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Buy, OrderType::Limit, 100.0, 5));

    auto fills = book.place_market_order(Order(2, Side::Sell, OrderType::Limit, 101.0, 5));

    CHECK(fills.size() == 1);
    CHECK(fills[0].quantity == 5);
    CHECK(fills[0].price == 100.0);
    CHECK(book.empty());
}

void test_limit_entrypoint_uses_limit_semantics() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Sell, OrderType::Limit, 101.0, 5));

    auto fills = book.place_limit_order(Order(2, Side::Buy, OrderType::Market, 100.0, 5));

    CHECK(fills.empty());
    CHECK(!book.empty());

    auto market_fills = book.place_market_order(Order(3, Side::Sell, OrderType::Market, 0.0, 5));
    CHECK(market_fills.size() == 1);
    CHECK(market_fills[0].buy_order_id == 2);
    CHECK(market_fills[0].price == 100.0);
}

void test_non_finite_limit_price_is_rejected() {
    OrderBook book;
    auto nan_fills = book.place_limit_order(
        Order(1, Side::Buy, OrderType::Limit, std::numeric_limits<double>::quiet_NaN(), 5));
    auto positive_infinity_fills = book.place_limit_order(
        Order(2, Side::Buy, OrderType::Limit, std::numeric_limits<double>::infinity(), 5));
    auto negative_infinity_fills = book.place_limit_order(
        Order(3, Side::Sell, OrderType::Limit, -std::numeric_limits<double>::infinity(), 5));

    CHECK(nan_fills.empty());
    CHECK(positive_infinity_fills.empty());
    CHECK(negative_infinity_fills.empty());
    CHECK(book.empty());
}

int main() {
    test_limit_order_no_match();
    test_limit_order_full_fill();
    test_limit_order_partial_fill();
    test_limit_partial_fill_rests_remainder();
    test_multi_level_fill();
    test_market_order_full_fill();
    test_market_order_partial_fill();
    test_cancel_order();
    test_cancel_nonexistent_order();
    test_cancel_partial_fill_level();
    test_partial_fill_with_remainder();
    test_best_price_priority();
    test_time_priority_at_same_price();
    test_duplicate_active_order_id_is_rejected();
    test_market_entrypoint_uses_market_semantics();
    test_limit_entrypoint_uses_limit_semantics();
    test_non_finite_limit_price_is_rejected();

    std::cout << "All matching tests passed!" << std::endl;
    return 0;
}
