#include <iostream>
#include <vector>

#include "order_book.h"

using namespace orderbook;

int main() {
    OrderBook book;
    std::vector<OrderID> order_ids;

    const int NUM_ORDERS = 1000000;

    // Benchmark: place 1M limit orders
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_ORDERS; ++i) {
        Price price = 100.0 + (i % 20);  // 20 price levels
        Order order(i, Side::Buy, OrderType::Limit, price, 10);
        book.place_limit_order(order);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double throughput = (double)NUM_ORDERS / (duration.count() / 1000.0);
    std::cout << "Limit orders placed: " << NUM_ORDERS << std::endl;
    std::cout << "Throughput: " << throughput << " orders/sec" << std::endl;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;

    return 0;
}
