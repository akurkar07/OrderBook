#include "order_book.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

namespace orderbook {
namespace {

bool is_valid_side(Side side) {
    return side == Side::Buy || side == Side::Sell;
}

bool is_valid_limit_order(const Order& order) {
    return is_valid_side(order.side) && order.quantity > 0 &&
           std::isfinite(order.price) && order.price > 0.0;
}

bool is_valid_market_order(const Order& order) {
    return is_valid_side(order.side) && order.quantity > 0;
}

bool can_rest_limit_remainder(const Order& order,
                              const std::map<Price, PriceLevel>& buy_levels,
                              const std::map<Price, PriceLevel>& sell_levels) {
    const auto& same_side_levels =
        order.side == Side::Buy ? buy_levels : sell_levels;
    const auto same_level = same_side_levels.find(order.price);
    if (same_level == same_side_levels.end()) {
        return true;
    }

    Quantity remaining = order.quantity;

    if (order.side == Side::Buy) {
        for (auto it = sell_levels.begin();
             it != sell_levels.end() && it->first <= order.price && remaining > 0;
             ++it) {
            const Quantity available = it->second.total_quantity();
            if (available >= remaining) {
                remaining = 0;
                break;
            }
            remaining -= available;
        }
    } else {
        for (auto it = buy_levels.rbegin();
             it != buy_levels.rend() && it->first >= order.price && remaining > 0;
             ++it) {
            const Quantity available = it->second.total_quantity();
            if (available >= remaining) {
                remaining = 0;
                break;
            }
            remaining -= available;
        }
    }

    return remaining <= std::numeric_limits<Quantity>::max() -
                            same_level->second.total_quantity();
}

} // namespace

OrderBook::OrderBook() : next_fill_id_(0) {}

Fills OrderBook::place_limit_order(const Order& order) {
    if (!is_valid_limit_order(order) ||
        order_id_to_price_.find(order.id) != order_id_to_price_.end() ||
        !can_rest_limit_remainder(order, buy_levels_, sell_levels_)) {
        return {};
    }

    Order working_order = order;
    working_order.type = OrderType::Limit;
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
    if (!is_valid_market_order(order) ||
        order_id_to_price_.find(order.id) != order_id_to_price_.end()) {
        return {};
    }

    Order working_order = order;
    working_order.type = OrderType::Market;
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
    bool is_buy = order_is_buy_.at(order_id);

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
            const Order& resting = level.front();
            OrderID resting_id = resting.id;
            Quantity fill_qty = std::min(order.quantity, resting.quantity);

            Fill fill;
            fill.id = next_fill_id_++;
            fill.buy_order_id = resting_id;
            fill.sell_order_id = order.id;
            fill.price = level.price();
            fill.quantity = fill_qty;
            fills.push_back(fill);

            order.quantity -= fill_qty;
            level.reduce_quantity(fill_qty);

            if (level.front().quantity == 0) {
                order_id_to_price_.erase(resting_id);
                order_is_buy_.erase(resting_id);
                level.remove_order(resting_id);
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
            const Order& resting = level.front();
            OrderID resting_id = resting.id;
            Quantity fill_qty = std::min(order.quantity, resting.quantity);

            Fill fill;
            fill.id = next_fill_id_++;
            fill.buy_order_id = order.id;
            fill.sell_order_id = resting_id;
            fill.price = level.price();
            fill.quantity = fill_qty;
            fills.push_back(fill);

            order.quantity -= fill_qty;
            level.reduce_quantity(fill_qty);

            if (level.front().quantity == 0) {
                order_id_to_price_.erase(resting_id);
                order_is_buy_.erase(resting_id);
                level.remove_order(resting_id);
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
