#ifndef ORDERBOOK_ORDER_H
#define ORDERBOOK_ORDER_H

#include <chrono>
#include <cstdint>

namespace orderbook {

using OrderID = uint64_t;
using Price = double;
using Quantity = uint64_t;
using Timestamp = std::chrono::steady_clock::time_point;

enum class Side : uint8_t {
    Buy,
    Sell
};

enum class OrderType : uint8_t {
    Limit,
    Market
};

struct Order {
    OrderID id{0};
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};
    Price price{0.0};
    Quantity quantity{0};
    Timestamp timestamp{};

    Order() = default;

    Order(OrderID id, Side side, OrderType type, Price price, Quantity quantity)
        : id(id), side(side), type(type), price(price), quantity(quantity),
          timestamp(std::chrono::steady_clock::now()) {}
};

} // namespace orderbook

#endif // ORDERBOOK_ORDER_H
