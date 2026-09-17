#include <chrono>
#include <iostream>

#include "order_book.h"

using namespace orderbook;

int main() {
    OrderBook book;

    constexpr int num_orders = 1000000;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_orders; ++i) {
        Price price = 100.0 + (i % 20);
        Order order(i, Side::Buy, OrderType::Limit, price, 10);
        book.place_limit_order(order);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double seconds = static_cast<double>(duration.count()) / 1'000'000'000.0;
    double throughput = static_cast<double>(num_orders) / seconds;
    std::cout << "Limit orders placed: " << num_orders << std::endl;
    std::cout << "Throughput: " << throughput << " orders/sec" << std::endl;
    std::cout << "Total time: " << seconds * 1000.0 << " ms" << std::endl;

    return 0;
}
