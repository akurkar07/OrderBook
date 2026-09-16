# OrderBook

A C++ limit order book engine with price-time priority matching, deterministic tests, and a benchmark harness.

## Overview

This project implements a price-time priority matching engine for a single instrument. It supports limit orders, market orders, and cancellations, with all operations designed to be deterministic and reproducible.

The central research question is: **How do data structure choices and memory layout decisions affect matching throughput and latency in a single-instrument order book?**

## Architecture

```
OrderBook/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── src/
│   ├── order_book.h      # Core OrderBook class
│   ├── order_book.cpp
│   ├── price_level.h     # PriceLevel with FIFO queue
│   ├── price_level.cpp
│   ├── order.h           # Order struct and types
│   └── types.h           # Common typedefs
├── tests/
│   ├── test_order_book.cpp
│   ├── test_price_level.cpp
│   └── test_matching.cpp
├── benchmarks/
│   ├── benchmark_order_book.cpp
│   └── reference_book.h  # Naive reference implementation
└── scripts/
    └── run_benchmarks.sh
```

## Build

```bash
git clone https://github.com/akurkar07/OrderBook.git
cd OrderBook
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Run Tests

```bash
cd build
ctest --output-on-failure
```

## Run Benchmarks

```bash
cd build
./benchmarks/benchmark_order_book
```

## Design Decisions

- **Price levels**: `std::map` (red-black tree) for O(log N) price lookup
- **Orders per level**: `std::list` for O(1) FIFO insertion and cancellation
- **Order ID lookup**: `std::unordered_map` for O(1) cancellation
- **Deterministic**: Same order sequence always produces same fill sequence
- **No dynamic memory in hot path**: Pre-allocated pools for orders (stretch goal)

## Milestones

### V1: Core Matching Engine
- [ ] OrderBook class with add/cancel/match
- [ ] PriceLevel with FIFO queue
- [ ] Limit order matching
- [ ] Market order matching
- [ ] Deterministic test suite

### V2: Verification & Benchmarking
- [ ] Naive reference implementation (vector-based)
- [ ] Correctness verification: same sequence → same fills
- [ ] Benchmark harness: throughput and latency
- [ ] Performance baseline

### V3: Optimization & Polish
- [ ] Memory pool allocator
- [ ] Cache-friendly data layout
- [ ] CI/CD with GitHub Actions
- [ ] Documentation and examples

## License

MIT
