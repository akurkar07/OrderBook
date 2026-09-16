#ifndef ORDERBOOK_TYPES_H
#define ORDERBOOK_TYPES_H

#include <cstdint>
#include <vector>

namespace orderbook {

using FillID = uint64_t;

struct Fill {
    FillID id;
    OrderID buy_order_id;
    OrderID sell_order_id;
    Price price;
    Quantity quantity;
};

using Fills = std::vector<Fill>;

} // namespace orderbook

#endif // ORDERBOOK_TYPES_H
