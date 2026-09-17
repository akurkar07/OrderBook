# OrderBook

A C++ limit order book engine with price-time priority matching, deterministic tests, differential verification, and a benchmark harness.

## Overview

This project implements a price-time priority matching engine for a single instrument. It supports limit orders, market orders, and cancellations, with all operations designed to be deterministic and reproducible.

The central research question is: **How do data structure choices and memory layout decisions affect matching throughput and latency in a single-instrument order book?**

## Architecture

```text
OrderBook/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── src/
│   ├── order_book.h
│   ├── order_book.cpp
│   ├── price_level.h
│   ├── price_level.cpp
│   ├── order.h
│   └── types.h
├── reference/
│   └── reference_order_book.h
├── tests/
│   ├── test_order_book.cpp
│   ├── test_price_level.cpp
│   ├── test_matching.cpp
│   ├── test_robustness.cpp
│   ├── test_reference_order_book.cpp
│   └── test_utils.h
└── benchmarks/
    └── benchmark_order_book.cpp
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

The test checks remain active in Release builds and do not depend on the standard `assert()` macro. The reference differential test also compares the optimized book against a deliberately simple vector-based model across deterministic randomized order sequences.

## Run Benchmarks

```bash
cd build
./benchmark_order_book
```

The default benchmark runs 1,000,000 optimized operations for each throughput workload, samples 100,000 operations per latency workload, and compares the optimized engine with the reference implementation over 10,000 identical operations.

Measured workloads:

- limit order placement throughput
- market order matching throughput
- cancellation throughput
- p50, p99 and p999 latency for each workload
- latency histograms in nanoseconds
- optimized vs reference throughput and relative speedup

Throughput runs deliberately exclude setup and per-operation timing overhead. Latency samples include `steady_clock` measurement overhead, so absolute values are machine-dependent.

For a shorter CI or smoke run:

```bash
./benchmark_order_book --quick
```

Custom counts are also supported:

```bash
./benchmark_order_book --orders 1000000 --latency-samples 100000 --reference-orders 10000
```

## Design Decisions

- **Price levels**: `std::map` for O(log N) price lookup
- **Orders per level**: `std::list` for FIFO insertion and stable order storage
- **Order ID lookup**: `std::unordered_map` for O(1) average price-level lookup during cancellation, followed by a linear scan within that level
- **Deterministic**: The same order sequence produces the same fill sequence
- **Active order IDs**: Duplicate IDs are rejected while the original order is still resting
- **Order validation**: Orders require a valid buy/sell side and non-zero quantity; limit prices must also be finite and strictly positive
- **Price-level invariants**: A level has a finite positive price and accepts only valid limit orders at exactly that price
- **Quantity accounting**: Price-level aggregate quantity overflow is rejected before mutation; `OrderBook` rejects an order that would overflow a resting level instead of leaking an internal exception
- **Reference verification**: A flat-vector reference implementation scans for the best eligible resting order on every fill so its structure is independent from the optimized engine
- **Benchmark methodology**: Bulk throughput and per-operation latency are measured separately to avoid clock sampling distorting the throughput figures

## Milestones

### V1: Core Matching Engine
- [x] OrderBook class with add/cancel/match
- [x] PriceLevel with FIFO queue
- [x] Limit order matching
- [x] Market order matching
- [x] Deterministic test suite

### V2: Verification & Benchmarking
- [x] Naive reference implementation (vector-based)
- [x] Correctness verification: same sequence → same fills
- [x] Benchmark harness: throughput and latency
- [x] Performance baseline harness

### V3: Optimization & Polish
- [ ] Memory pool allocator
- [ ] Cache-friendly data layout
- [ ] CI/CD with GitHub Actions
- [ ] Documentation and examples

## License

MIT
