#include "price_level.h"

#include <algorithm>
#include <cassert>

namespace orderbook {

PriceLevel::PriceLevel(Price price) : price_(price), total_quantity_(0) {}

void PriceLevel::add_order(const Order& order) {
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
    assert(!queue_.empty());
    if (queue_.empty()) {
        return;
    }

    assert(amount <= queue_.front().quantity);
    if (amount > queue_.front().quantity) {
        return;
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
    return queue_.front();
}

} // namespace orderbook
