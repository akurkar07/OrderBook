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

int main() {
    test_limit_order_no_match();
    test_limit_order_full_fill();
    test_limit_order_partial_fill();
    test_market_order_full_fill();
    test_market_order_partial_fill();
    test_cancel_order();
    test_cancel_nonexistent_order();
    test_cancel_partial_fill_level();

    std::cout << "All matching tests passed!" << std::endl;
    return 0;
}
