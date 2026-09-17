#ifndef ORDERBOOK_TYPES_H
#define ORDERBOOK_TYPES_H

#include "order.h"

#include <cstdint>
#include <vector>

namespace orderbook {

using FillID = uint64_t;

struct Fill {
    FillID id{0};
    OrderID buy_order_id{0};
    OrderID sell_order_id{0};
    Price price{0.0};
    Quantity quantity{0};
};

using Fills = std::vector<Fill>;

} // namespace orderbook

#endif // ORDERBOOK_TYPES_H
