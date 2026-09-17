#include <cassert>
#include <iostream>
#include <limits>

#include "order_book.h"

using namespace orderbook;

void test_limit_order_no_match() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    auto fills = book.place_limit_order(buy);

    assert(fills.empty());
    assert(!book.empty());

    Order sell(2, Side::Sell, OrderType::Limit, 101.0, 5);
    fills = book.place_limit_order(sell);
    assert(fills.empty());
}

void test_limit_order_full_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    Order sell(2, Side::Sell, OrderType::Limit, 100.0, 10);
    auto fills = book.place_limit_order(sell);

    assert(fills.size() == 1);
    assert(fills[0].quantity == 10);
    assert(fills[0].price == 100.0);
    assert(fills[0].buy_order_id == 1);
    assert(fills[0].sell_order_id == 2);
    assert(book.empty());
}

void test_limit_order_partial_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    Order sell(2, Side::Sell, OrderType::Limit, 100.0, 3);
    auto fills = book.place_limit_order(sell);

    assert(fills.size() == 1);
    assert(fills[0].quantity == 3);
    assert(!book.empty());

    Order sell2(3, Side::Sell, OrderType::Limit, 100.0, 7);
    fills = book.place_limit_order(sell2);
    assert(fills.size() == 1);
    assert(fills[0].quantity == 7);
    assert(book.empty());
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

    assert(fills.size() == 3);
    assert(fills[0].quantity == 5);
    assert(fills[0].price == 100.0);
    assert(fills[1].quantity == 10);
    assert(fills[1].price == 99.0);
    assert(fills[2].quantity == 5);
    assert(fills[2].price == 98.0);
    assert(!book.empty());
}

void test_market_order_full_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    Order market_sell(2, Side::Sell, OrderType::Market, 0.0, 10);
    auto fills = book.place_market_order(market_sell);

    assert(fills.size() == 1);
    assert(fills[0].quantity == 10);
    assert(fills[0].price == 100.0);
    assert(book.empty());
}

void test_market_order_partial_fill() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 5);
    book.place_limit_order(buy);

    Order market_sell(2, Side::Sell, OrderType::Market, 0.0, 10);
    auto fills = book.place_market_order(market_sell);

    assert(fills.size() == 1);
    assert(fills[0].quantity == 5);
    assert(book.empty());
}

void test_cancel_order() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    book.place_limit_order(buy);

    assert(book.cancel_order(1));
    assert(book.empty());
}

void test_cancel_nonexistent_order() {
    OrderBook book;
    assert(!book.cancel_order(999));
}

void test_cancel_partial_fill_level() {
    OrderBook book;
    Order buy1(1, Side::Buy, OrderType::Limit, 100.0, 5);
    Order buy2(2, Side::Buy, OrderType::Limit, 100.0, 5);
    book.place_limit_order(buy1);
    book.place_limit_order(buy2);

    assert(book.cancel_order(1));
    assert(!book.empty());

    assert(book.cancel_order(2));
    assert(book.empty());
}

void test_partial_fill_with_remainder() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 5);
    book.place_limit_order(buy);

    Order market_sell(2, Side::Sell, OrderType::Market, 0.0, 10);
    auto fills = book.place_market_order(market_sell);

    assert(fills.size() == 1);
    assert(fills[0].quantity == 5);
    assert(book.empty());
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

    assert(fills.size() == 1);
    assert(fills[0].price == 100.0);
    assert(fills[0].quantity == 5);
}

void test_time_priority_at_same_price() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Sell, OrderType::Limit, 100.0, 3));
    book.place_limit_order(Order(2, Side::Sell, OrderType::Limit, 100.0, 4));

    auto fills = book.place_market_order(Order(3, Side::Buy, OrderType::Market, 0.0, 5));

    assert(fills.size() == 2);
    assert(fills[0].sell_order_id == 1);
    assert(fills[0].quantity == 3);
    assert(fills[1].sell_order_id == 2);
    assert(fills[1].quantity == 2);
}

void test_duplicate_active_order_id_is_rejected() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Buy, OrderType::Limit, 100.0, 5));
    auto duplicate_fills = book.place_limit_order(Order(1, Side::Buy, OrderType::Limit, 99.0, 5));

    assert(duplicate_fills.empty());
    assert(book.cancel_order(1));

    auto fills = book.place_market_order(Order(2, Side::Sell, OrderType::Market, 0.0, 10));
    assert(fills.empty());
    assert(book.empty());
}

void test_market_entrypoint_uses_market_semantics() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Buy, OrderType::Limit, 100.0, 5));

    auto fills = book.place_market_order(Order(2, Side::Sell, OrderType::Limit, 101.0, 5));

    assert(fills.size() == 1);
    assert(fills[0].quantity == 5);
    assert(fills[0].price == 100.0);
    assert(book.empty());
}

void test_limit_entrypoint_uses_limit_semantics() {
    OrderBook book;
    book.place_limit_order(Order(1, Side::Sell, OrderType::Limit, 101.0, 5));

    auto fills = book.place_limit_order(Order(2, Side::Buy, OrderType::Market, 100.0, 5));

    assert(fills.empty());
    assert(!book.empty());

    auto market_fills = book.place_market_order(Order(3, Side::Sell, OrderType::Market, 0.0, 5));
    assert(market_fills.size() == 1);
    assert(market_fills[0].buy_order_id == 2);
    assert(market_fills[0].price == 100.0);
}

void test_non_finite_limit_price_is_rejected() {
    OrderBook book;
    auto fills = book.place_limit_order(
        Order(1, Side::Buy, OrderType::Limit, std::numeric_limits<double>::quiet_NaN(), 5));

    assert(fills.empty());
    assert(book.empty());
}

int main() {
    test_limit_order_no_match();
    test_limit_order_full_fill();
    test_limit_order_partial_fill();
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
