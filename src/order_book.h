#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "order.h"
#include "types.h"
#include "price_level.h"
#include <map>
#include <unordered_map>

namespace orderbook {

class OrderBook {
public:
    OrderBook();

    Fills place_limit_order(const Order& order);
    Fills place_market_order(const Order& order);
    bool cancel_order(OrderID order_id);

    bool empty() const;

private:
    Fills match_against_buy_levels(Order& order);
    Fills match_against_sell_levels(Order& order);

    // Buy levels: sorted ascending, best buy = highest price (rbegin)
    std::map<Price, PriceLevel> buy_levels_;
    // Sell levels: sorted ascending, best sell = lowest price (begin)
    std::map<Price, PriceLevel> sell_levels_;

    // Order ID to price level lookup for O(1) cancellation
    std::unordered_map<OrderID, Price> order_id_to_price_;
    std::unordered_map<OrderID, bool> order_is_buy_;  // true if buy, false if sell

    FillID next_fill_id_;
};

} // namespace orderbook

#endif // ORDERBOOK_H
