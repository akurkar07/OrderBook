#include <cassert>
#include <iostream>
#include <algorithm>

#include "order_book.h"

using namespace orderbook;

void test_limit_order_no_match() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 10);
    auto fills = book.place_limit_order(buy);

    assert(fills.empty());
    assert(!book.empty());

    // No match for sell below buy price
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
    assert(!book.empty());  // Buy order still has 7 remaining

    // Fill the rest
    Order sell2(3, Side::Sell, OrderType::Limit, 100.0, 7);
    fills = book.place_limit_order(sell2);
    assert(fills.size() == 1);
    assert(fills[0].quantity == 7);
    assert(book.empty());
}

void test_multi_level_fill() {
    OrderBook book;
    // Place buys at multiple price levels
    Order buy1(1, Side::Buy, OrderType::Limit, 100.0, 5);
    Order buy2(2, Side::Buy, OrderType::Limit, 99.0, 10);
    Order buy3(3, Side::Buy, OrderType::Limit, 98.0, 15);
    book.place_limit_order(buy1);
    book.place_limit_order(buy2);
    book.place_limit_order(buy3);

    // Incoming sell that should fill across all levels
    Order sell(4, Side::Sell, OrderType::Limit, 98.0, 20);
    auto fills = book.place_limit_order(sell);

    assert(fills.size() == 3);
    // First fill: 5 @ 100.0 (best price)
    assert(fills[0].quantity == 5);
    assert(fills[0].price == 100.0);
    // Second fill: 10 @ 99.0
    assert(fills[1].quantity == 10);
    assert(fills[1].price == 99.0);
    // Third fill: 5 @ 98.0 (partial)
    assert(fills[2].quantity == 5);
    assert(fills[2].price == 98.0);

    // Book should still have buy3 with 10 remaining
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
    assert(fills[0].quantity == 5);  // Only 5 available
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
    // Level still has buy2
    assert(!book.empty());

    assert(book.cancel_order(2));
    assert(book.empty());
}

void test_partial_fill_with_remainder() {
    OrderBook book;
    Order buy(1, Side::Buy, OrderType::Limit, 100.0, 5);
    book.place_limit_order(buy);

    // Sell more than available - partial fill, rest cancelled (market order)
    Order market_sell(2, Side::Sell, OrderType::Market, 0.0, 10);
    auto fills = book.place_market_order(market_sell);

    assert(fills.size() == 1);
    assert(fills[0].quantity == 5);
    assert(book.empty());
}

void test_best_price_priority() {
    OrderBook book;
    // Place sells at different prices
    Order sell1(1, Side::Sell, OrderType::Limit, 101.0, 5);
    Order sell2(2, Side::Sell, OrderType::Limit, 100.0, 5);
    Order sell3(3, Side::Sell, OrderType::Limit, 102.0, 5);
    book.place_limit_order(sell1);
    book.place_limit_order(sell2);
    book.place_limit_order(sell3);

    // Buy should match best (lowest) sell price first
    Order buy(4, Side::Buy, OrderType::Limit, 101.0, 5);
    auto fills = book.place_limit_order(buy);

    assert(fills.size() == 1);
    assert(fills[0].price == 100.0);  // Best price
    assert(fills[0].quantity == 5);
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

    std::cout << "All matching tests passed!" << std::endl;
    return 0;
}
