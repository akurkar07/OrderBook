#ifndef ORDERBOOK_REFERENCE_ORDER_BOOK_H
#define ORDERBOOK_REFERENCE_ORDER_BOOK_H

#include "order.h"
#include "types.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace orderbook {

class ReferenceOrderBook {
public:
    Fills place_limit_order(const Order& order) {
        Order working = order;
        working.type = OrderType::Limit;
        if (!is_valid_limit_order(order) || is_active(order.id) ||
            !can_rest_limit_remainder(working)) {
            return {};
        }

        Fills fills = match(working);
        if (working.quantity > 0) {
            resting_orders(working.side).push_back(working);
        }
        return fills;
    }

    Fills place_market_order(const Order& order) {
        if (!is_valid_market_order(order) || is_active(order.id)) {
            return {};
        }

        Order working = order;
        working.type = OrderType::Market;
        return match(working);
    }

    bool cancel_order(OrderID order_id) {
        for (auto* side_orders : {&buys_, &sells_}) {
            auto it = std::find_if(side_orders->begin(), side_orders->end(),
                                   [order_id](const Order& order) {
                                       return order.id == order_id;
                                   });
            if (it != side_orders->end()) {
                side_orders->erase(it);
                return true;
            }
        }
        return false;
    }

    bool empty() const {
        return buys_.empty() && sells_.empty();
    }

private:
    static bool is_valid_side(Side side) {
        return side == Side::Buy || side == Side::Sell;
    }

    static bool is_valid_limit_order(const Order& order) {
        return is_valid_side(order.side) && order.quantity > 0 &&
               std::isfinite(order.price) && order.price > 0.0;
    }

    static bool is_valid_market_order(const Order& order) {
        return is_valid_side(order.side) && order.quantity > 0;
    }

    bool is_active(OrderID order_id) const {
        const auto has_id = [order_id](const std::vector<Order>& orders) {
            return std::any_of(orders.begin(), orders.end(),
                               [order_id](const Order& order) {
                                   return order.id == order_id;
                               });
        };
        return has_id(buys_) || has_id(sells_);
    }

    std::vector<Order>& resting_orders(Side side) {
        return side == Side::Buy ? buys_ : sells_;
    }

    const std::vector<Order>& resting_orders(Side side) const {
        return side == Side::Buy ? buys_ : sells_;
    }

    Quantity same_price_total(Side side, Price price) const {
        Quantity total = 0;
        for (const Order& order : resting_orders(side)) {
            if (order.price == price) {
                total += order.quantity;
            }
        }
        return total;
    }

    Quantity crossed_quantity(const Order& order) const {
        Quantity remaining = order.quantity;
        std::vector<Order> opposite =
            order.side == Side::Buy ? sells_ : buys_;

        while (remaining > 0) {
            const std::size_t index = best_match_index(order, opposite);
            if (index == npos) {
                break;
            }

            const Quantity fill = std::min(remaining, opposite[index].quantity);
            remaining -= fill;
            opposite[index].quantity -= fill;
            if (opposite[index].quantity == 0) {
                opposite.erase(opposite.begin() + static_cast<std::ptrdiff_t>(index));
            }
        }

        return order.quantity - remaining;
    }

    bool can_rest_limit_remainder(const Order& order) const {
        const Quantity same_total = same_price_total(order.side, order.price);
        if (same_total == 0) {
            return true;
        }

        const Quantity crossed = crossed_quantity(order);
        const Quantity remainder = order.quantity - crossed;
        return remainder <= std::numeric_limits<Quantity>::max() - same_total;
    }

    Fills match(Order& incoming) {
        Fills fills;
        auto& opposite = incoming.side == Side::Buy ? sells_ : buys_;

        while (incoming.quantity > 0) {
            const std::size_t index = best_match_index(incoming, opposite);
            if (index == npos) {
                break;
            }

            Order& resting = opposite[index];
            const Quantity fill_quantity = std::min(incoming.quantity, resting.quantity);

            Fill fill;
            fill.id = next_fill_id_++;
            if (incoming.side == Side::Buy) {
                fill.buy_order_id = incoming.id;
                fill.sell_order_id = resting.id;
            } else {
                fill.buy_order_id = resting.id;
                fill.sell_order_id = incoming.id;
            }
            fill.price = resting.price;
            fill.quantity = fill_quantity;
            fills.push_back(fill);

            incoming.quantity -= fill_quantity;
            resting.quantity -= fill_quantity;
            if (resting.quantity == 0) {
                opposite.erase(opposite.begin() + static_cast<std::ptrdiff_t>(index));
            }
        }

        return fills;
    }

    std::size_t best_match_index(const Order& incoming,
                                 const std::vector<Order>& opposite) const {
        std::size_t best = npos;
        for (std::size_t i = 0; i < opposite.size(); ++i) {
            const Order& candidate = opposite[i];
            if (!crosses(incoming, candidate)) {
                continue;
            }

            if (best == npos || better_price(incoming.side, candidate.price,
                                             opposite[best].price)) {
                best = i;
            }
        }
        return best;
    }

    static bool crosses(const Order& incoming, const Order& resting) {
        if (incoming.type == OrderType::Market) {
            return true;
        }
        if (incoming.side == Side::Buy) {
            return resting.price <= incoming.price;
        }
        return resting.price >= incoming.price;
    }

    static bool better_price(Side incoming_side, Price candidate, Price current) {
        if (incoming_side == Side::Buy) {
            return candidate < current;
        }
        return candidate > current;
    }

    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

    std::vector<Order> buys_;
    std::vector<Order> sells_;
    FillID next_fill_id_{0};
};

} // namespace orderbook

#endif // ORDERBOOK_REFERENCE_ORDER_BOOK_H
