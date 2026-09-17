#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include "order_book.h"
#include "reference_order_book.h"

using namespace orderbook;

namespace {

bool same_fills(const Fills& lhs, const Fills& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i].id != rhs[i].id ||
            lhs[i].buy_order_id != rhs[i].buy_order_id ||
            lhs[i].sell_order_id != rhs[i].sell_order_id ||
            lhs[i].price != rhs[i].price ||
            lhs[i].quantity != rhs[i].quantity) {
            return false;
        }
    }
    return true;
}

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Reference test failed: " << message << '\n';
        std::abort();
    }
}

void compare_limit(OrderBook& book, ReferenceOrderBook& reference, const Order& order) {
    require(same_fills(book.place_limit_order(order), reference.place_limit_order(order)),
            "limit fills differ");
}

void compare_market(OrderBook& book, ReferenceOrderBook& reference, const Order& order) {
    require(same_fills(book.place_market_order(order), reference.place_market_order(order)),
            "market fills differ");
}

void compare_cancel(OrderBook& book, ReferenceOrderBook& reference, OrderID order_id) {
    require(book.cancel_order(order_id) == reference.cancel_order(order_id),
            "cancellation result differs");
}

void test_deterministic_sequence() {
    OrderBook book;
    ReferenceOrderBook reference;

    compare_limit(book, reference, Order(1, Side::Sell, OrderType::Limit, 101.0, 4));
    compare_limit(book, reference, Order(2, Side::Sell, OrderType::Limit, 100.0, 3));
    compare_limit(book, reference, Order(3, Side::Sell, OrderType::Limit, 100.0, 5));
    compare_limit(book, reference, Order(4, Side::Buy, OrderType::Limit, 100.0, 6));
    compare_market(book, reference, Order(5, Side::Buy, OrderType::Market, 0.0, 10));
    compare_limit(book, reference, Order(6, Side::Buy, OrderType::Limit, 99.0, 7));
    compare_limit(book, reference, Order(7, Side::Buy, OrderType::Limit, 98.0, 2));
    compare_cancel(book, reference, 6);
    compare_market(book, reference, Order(8, Side::Sell, OrderType::Market, 0.0, 5));

    require(book.empty() == reference.empty(), "final empty state differs");
}

void test_random_sequences() {
    constexpr std::uint64_t seeds = 50;
    constexpr int operations_per_seed = 2000;

    for (std::uint64_t seed = 0; seed < seeds; ++seed) {
        std::mt19937_64 rng(0xC0FFEEULL + seed);
        OrderBook book;
        ReferenceOrderBook reference;
        OrderID next_id = 1;
        std::vector<OrderID> seen_ids;

        for (int step = 0; step < operations_per_seed; ++step) {
            const int action = static_cast<int>(rng() % 100);

            if (action < 60) {
                const Side side = (rng() & 1U) ? Side::Buy : Side::Sell;
                const Price price = 95.0 + static_cast<Price>(rng() % 11);
                const Quantity quantity = 1 + static_cast<Quantity>(rng() % 25);
                OrderID id = next_id++;
                if (!seen_ids.empty() && rng() % 10 == 0) {
                    id = seen_ids[rng() % seen_ids.size()];
                }
                seen_ids.push_back(id);
                compare_limit(book, reference,
                              Order(id, side, OrderType::Limit, price, quantity));
            } else if (action < 85) {
                const Side side = (rng() & 1U) ? Side::Buy : Side::Sell;
                const Quantity quantity = 1 + static_cast<Quantity>(rng() % 40);
                OrderID id = next_id++;
                if (!seen_ids.empty() && rng() % 12 == 0) {
                    id = seen_ids[rng() % seen_ids.size()];
                }
                seen_ids.push_back(id);
                compare_market(book, reference,
                               Order(id, side, OrderType::Market, 0.0, quantity));
            } else {
                OrderID id = next_id + static_cast<OrderID>(rng() % 50);
                if (!seen_ids.empty() && rng() % 4 != 0) {
                    id = seen_ids[rng() % seen_ids.size()];
                }
                compare_cancel(book, reference, id);
            }

            require(book.empty() == reference.empty(), "empty state differs");
        }
    }
}

} // namespace

int main() {
    test_deterministic_sequence();
    test_random_sequences();
    std::cout << "Reference differential tests passed!" << std::endl;
    return 0;
}
