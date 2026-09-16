#ifndef ORDERBOOK_PRICE_LEVEL_H
#define ORDERBOOK_PRICE_LEVEL_H

#include "order.h"
#include <list>

namespace orderbook {

class PriceLevel {
public:
    explicit PriceLevel(Price price);

    void add_order(const Order& order);
    bool remove_order(OrderID order_id);
    void reduce_quantity(Quantity amount);
    bool empty() const;

    Quantity total_quantity() const;
    Price price() const;

    const Order& front() const;
    Order& front();

private:
    Price price_;
    std::list<Order> queue_;  // FIFO queue for time priority
    Quantity total_quantity_;  // Cached total quantity
};

} // namespace orderbook

#endif // ORDERBOOK_PRICE_LEVEL_H
