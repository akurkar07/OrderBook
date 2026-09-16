#include "order_book.h"

namespace orderbook {

OrderBook::OrderBook() : next_fill_id_(0) {}

Fills OrderBook::place_limit_order(const Order& order) {
    Order working_order = order;
    Fills fills;

    if (order.side == Side::Buy) {
        fills = match_against_sell_levels(working_order);
    } else {
        fills = match_against_buy_levels(working_order);
    }

    // If there's remaining quantity, rest the order in the book
    if (working_order.quantity > 0) {
        if (order.side == Side::Buy) {
            auto result = buy_levels_.insert({order.price, PriceLevel(order.price)});
            result.first->second.add_order(working_order);
            order_id_to_price_[order.id] = order.price;
            order_is_buy_[order.id] = true;
        } else {
            auto result = sell_levels_.insert({order.price, PriceLevel(order.price)});
            result.first->second.add_order(working_order);
            order_id_to_price_[order.id] = order.price;
            order_is_buy_[order.id] = false;
        }
    }

    return fills;
}

Fills OrderBook::place_market_order(const Order& order) {
    Order working_order = order;
    Fills fills;

    if (order.side == Side::Buy) {
        fills = match_against_sell_levels(working_order);
    } else {
        fills = match_against_buy_levels(working_order);
    }

    // Market orders don't rest - any remaining quantity is cancelled
    return fills;
}

bool OrderBook::cancel_order(OrderID order_id) {
    auto it = order_id_to_price_.find(order_id);
    if (it == order_id_to_price_.end()) {
        return false;
    }

    Price price = it->second;
    bool is_buy = order_is_buy_[order_id];

    auto& levels = is_buy ? buy_levels_ : sell_levels_;
    auto level_it = levels.find(price);
    if (level_it != levels.end()) {
        bool removed = level_it->second.remove_order(order_id);
        if (removed) {
            order_id_to_price_.erase(order_id);
            order_is_buy_.erase(order_id);
            if (level_it->second.empty()) {
                levels.erase(level_it);
            }
            return true;
        }
    }

    return false;
}

bool OrderBook::empty() const {
    return buy_levels_.empty() && sell_levels_.empty();
}

Fills OrderBook::match_against_buy_levels(Order& order) {
    Fills fills;

    // Sell order matches against buy levels where buy price >= sell price
    // Best buy is the highest price (last element)
    while (!buy_levels_.empty() && order.quantity > 0) {
        auto it = std::prev(buy_levels_.end());  // highest buy price
        PriceLevel& level = it->second;

        if (level.price() < order.price && order.type == OrderType::Limit) {
            break;  // No more matching prices
        }

        // Match against orders at this level
        while (!level.empty() && order.quantity > 0) {
            Order& resting = level.front();
            Quantity fill_qty = std::min(order.quantity, resting.quantity);

            Fill fill;
            fill.id = next_fill_id_++;
            fill.buy_order_id = resting.id;
            fill.sell_order_id = order.id;
            fill.price = level.price();
            fill.quantity = fill_qty;
            fills.push_back(fill);

            order.quantity -= fill_qty;
            resting.quantity -= fill_qty;

            if (resting.quantity == 0) {
                order_id_to_price_.erase(resting.id);
                order_is_buy_.erase(resting.id);
                level.remove_order(resting.id);
            }
        }

        // Remove empty level
        if (level.empty()) {
            buy_levels_.erase(it);
        }
    }

    return fills;
}

Fills OrderBook::match_against_sell_levels(Order& order) {
    Fills fills;

    // Buy order matches against sell levels where sell price <= buy price
    // Best sell is the lowest price (first element)
    while (!sell_levels_.empty() && order.quantity > 0) {
        auto it = sell_levels_.begin();  // lowest sell price
        PriceLevel& level = it->second;

        if (level.price() > order.price && order.type == OrderType::Limit) {
            break;  // No more matching prices
        }

        // Match against orders at this level
        while (!level.empty() && order.quantity > 0) {
            Order& resting = level.front();
            Quantity fill_qty = std::min(order.quantity, resting.quantity);

            Fill fill;
            fill.id = next_fill_id_++;
            fill.buy_order_id = order.id;
            fill.sell_order_id = resting.id;
            fill.price = level.price();
            fill.quantity = fill_qty;
            fills.push_back(fill);

            order.quantity -= fill_qty;
            resting.quantity -= fill_qty;

            if (resting.quantity == 0) {
                order_id_to_price_.erase(resting.id);
                order_is_buy_.erase(resting.id);
                level.remove_order(resting.id);
            }
        }

        // Remove empty level
        if (level.empty()) {
            sell_levels_.erase(it);
        }
    }

    return fills;
}

} // namespace orderbook
