#include "price_level.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace orderbook {
namespace {

bool is_valid_side(Side side) {
    return side == Side::Buy || side == Side::Sell;
}

} // namespace

PriceLevel::PriceLevel(Price price) : price_(price), total_quantity_(0) {
    if (!std::isfinite(price_) || price_ <= 0.0) {
        throw std::invalid_argument("price level price must be finite and positive");
    }
}

void PriceLevel::add_order(const Order& order) {
    if (!is_valid_side(order.side) || order.type != OrderType::Limit ||
        order.quantity == 0 || !std::isfinite(order.price) ||
        order.price != price_) {
        throw std::invalid_argument("order is invalid for this price level");
    }

    if (order.quantity > std::numeric_limits<Quantity>::max() - total_quantity_) {
        throw std::overflow_error("price level quantity overflow");
    }

    queue_.push_back(order);
    total_quantity_ += order.quantity;
}

bool PriceLevel::remove_order(OrderID order_id) {
    auto it = std::find_if(queue_.begin(), queue_.end(),
                           [order_id](const Order& o) { return o.id == order_id; });
    if (it != queue_.end()) {
        total_quantity_ -= it->quantity;
        queue_.erase(it);
        return true;
    }
    return false;
}

void PriceLevel::reduce_quantity(Quantity amount) {
    if (queue_.empty()) {
        throw std::out_of_range("cannot reduce an empty price level");
    }

    if (amount > queue_.front().quantity) {
        throw std::invalid_argument("reduction exceeds front order quantity");
    }

    queue_.front().quantity -= amount;
    total_quantity_ -= amount;
}

bool PriceLevel::empty() const {
    return queue_.empty();
}

Quantity PriceLevel::total_quantity() const {
    return total_quantity_;
}

Price PriceLevel::price() const {
    return price_;
}

const Order& PriceLevel::front() const {
    if (queue_.empty()) {
        throw std::out_of_range("cannot access the front of an empty price level");
    }

    return queue_.front();
}

} // namespace orderbook
