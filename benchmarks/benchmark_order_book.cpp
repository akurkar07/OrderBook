#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "order_book.h"
#include "reference_order_book.h"

using namespace orderbook;

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::size_t kDefaultOrders = 1'000'000;
constexpr std::size_t kDefaultLatencySamples = 100'000;
constexpr std::size_t kDefaultReferenceOrders = 10'000;
constexpr std::size_t kQuickOrders = 100'000;
constexpr std::size_t kQuickLatencySamples = 10'000;
constexpr std::size_t kQuickReferenceOrders = 2'000;
constexpr std::size_t kPriceLevels = 64;
constexpr OrderID kIncomingIdBase = OrderID{1} << 62;

struct Config {
    std::size_t orders{kDefaultOrders};
    std::size_t latency_samples{kDefaultLatencySamples};
    std::size_t reference_orders{kDefaultReferenceOrders};
};

struct ThroughputResult {
    double seconds{0.0};
    double operations_per_second{0.0};
    std::uint64_t checksum{0};
};

struct LatencyResult {
    std::vector<std::uint64_t> nanoseconds;
    std::uint64_t checksum{0};
};

std::size_t parse_positive_count(const std::string& text, std::string_view option) {
    std::size_t parsed = 0;
    const unsigned long long value = std::stoull(text, &parsed);
    if (parsed != text.size() || value == 0 ||
        value > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
        throw std::invalid_argument(std::string(option) + " requires a positive integer");
    }
    return static_cast<std::size_t>(value);
}

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable << " [options]\n"
              << "  --orders N             optimized throughput operations (default 1000000)\n"
              << "  --latency-samples N    optimized latency samples per workload (default 100000)\n"
              << "  --reference-orders N   operations per implementation for reference comparison (default 10000)\n"
              << "  --quick                use CI-friendly counts (100000 / 10000 / 2000)\n"
              << "  --help                 show this message\n";
}

Config parse_config(int argc, char** argv) {
    Config config;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--quick") {
            config.orders = kQuickOrders;
            config.latency_samples = kQuickLatencySamples;
            config.reference_orders = kQuickReferenceOrders;
        } else if (argument == "--orders" || argument == "--latency-samples" ||
                   argument == "--reference-orders") {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string(argument) + " requires a value");
            }
            const std::size_t value = parse_positive_count(argv[++i], argument);
            if (argument == "--orders") {
                config.orders = value;
            } else if (argument == "--latency-samples") {
                config.latency_samples = value;
            } else {
                config.reference_orders = value;
            }
        } else {
            throw std::invalid_argument("unknown option: " + std::string(argument));
        }
    }
    return config;
}

Price passive_buy_price(std::size_t index) {
    return 99.0 - static_cast<double>(index % kPriceLevels) * 0.01;
}

Price ask_price(std::size_t level) {
    return 100.0 + static_cast<double>(level) * 0.01;
}

template <typename Book>
void seed_market_liquidity(Book& book, std::size_t operations) {
    const std::size_t levels = std::min(kPriceLevels, operations);
    const std::size_t base_quantity = operations / levels;
    const std::size_t remainder = operations % levels;

    for (std::size_t level = 0; level < levels; ++level) {
        const Quantity quantity = static_cast<Quantity>(
            base_quantity + (level < remainder ? std::size_t{1} : std::size_t{0}));
        book.place_limit_order(Order(static_cast<OrderID>(level + 1), Side::Sell,
                                     OrderType::Limit, ask_price(level), quantity));
    }
}

template <typename Book>
void seed_cancellation_orders(Book& book, std::size_t operations) {
    for (std::size_t i = 0; i < operations; ++i) {
        book.place_limit_order(Order(static_cast<OrderID>(i + 1), Side::Buy,
                                     OrderType::Limit, passive_buy_price(i), 1));
    }
}

ThroughputResult make_throughput_result(Clock::time_point start, Clock::time_point end,
                                        std::size_t operations, std::uint64_t checksum) {
    const double seconds = std::chrono::duration<double>(end - start).count();
    return {seconds, static_cast<double>(operations) / seconds, checksum};
}

template <typename Book>
ThroughputResult benchmark_limit_placement(std::size_t operations) {
    Book book;

    const auto start = Clock::now();
    for (std::size_t i = 0; i < operations; ++i) {
        book.place_limit_order(Order(static_cast<OrderID>(i + 1), Side::Buy,
                                     OrderType::Limit, passive_buy_price(i), 1));
    }
    const auto end = Clock::now();

    std::uint64_t checksum = 0;
    checksum += book.cancel_order(1) ? 1 : 0;
    if (operations > 1) {
        checksum += book.cancel_order(static_cast<OrderID>(operations)) ? 1 : 0;
    }
    return make_throughput_result(start, end, operations, checksum);
}

template <typename Book>
ThroughputResult benchmark_market_matching(std::size_t operations) {
    Book book;
    seed_market_liquidity(book, operations);

    std::uint64_t checksum = 0;
    const auto start = Clock::now();
    for (std::size_t i = 0; i < operations; ++i) {
        const Fills fills = book.place_market_order(
            Order(kIncomingIdBase + static_cast<OrderID>(i), Side::Buy,
                  OrderType::Market, 0.0, 1));
        checksum += static_cast<std::uint64_t>(fills.size());
        if (!fills.empty()) {
            checksum += fills.front().quantity;
        }
    }
    const auto end = Clock::now();

    return make_throughput_result(start, end, operations, checksum);
}

template <typename Book>
ThroughputResult benchmark_cancellation(std::size_t operations) {
    Book book;
    seed_cancellation_orders(book, operations);

    std::uint64_t checksum = 0;
    const auto start = Clock::now();
    for (std::size_t i = 0; i < operations; ++i) {
        checksum += book.cancel_order(static_cast<OrderID>(i + 1)) ? 1 : 0;
    }
    const auto end = Clock::now();

    return make_throughput_result(start, end, operations, checksum);
}

LatencyResult sample_limit_latency(std::size_t samples) {
    OrderBook book;
    LatencyResult result;
    result.nanoseconds.reserve(samples);

    for (std::size_t i = 0; i < samples; ++i) {
        const auto start = Clock::now();
        const Fills fills = book.place_limit_order(
            Order(static_cast<OrderID>(i + 1), Side::Buy, OrderType::Limit,
                  passive_buy_price(i), 1));
        const auto end = Clock::now();
        result.nanoseconds.push_back(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
        result.checksum += static_cast<std::uint64_t>(fills.size());
    }

    result.checksum += book.cancel_order(1) ? 1 : 0;
    return result;
}

LatencyResult sample_market_latency(std::size_t samples) {
    OrderBook book;
    seed_market_liquidity(book, samples);

    LatencyResult result;
    result.nanoseconds.reserve(samples);
    for (std::size_t i = 0; i < samples; ++i) {
        const auto start = Clock::now();
        const Fills fills = book.place_market_order(
            Order(kIncomingIdBase + static_cast<OrderID>(i), Side::Buy,
                  OrderType::Market, 0.0, 1));
        const auto end = Clock::now();
        result.nanoseconds.push_back(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
        result.checksum += static_cast<std::uint64_t>(fills.size());
    }
    return result;
}

LatencyResult sample_cancellation_latency(std::size_t samples) {
    OrderBook book;
    seed_cancellation_orders(book, samples);

    LatencyResult result;
    result.nanoseconds.reserve(samples);
    for (std::size_t i = 0; i < samples; ++i) {
        const auto start = Clock::now();
        const bool cancelled = book.cancel_order(static_cast<OrderID>(i + 1));
        const auto end = Clock::now();
        result.nanoseconds.push_back(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
        result.checksum += cancelled ? 1 : 0;
    }
    return result;
}

std::uint64_t percentile(const std::vector<std::uint64_t>& sorted, double fraction) {
    const double rank = std::ceil(fraction * static_cast<double>(sorted.size()));
    const std::size_t index = std::min(
        sorted.size() - 1,
        rank <= 1.0 ? std::size_t{0} : static_cast<std::size_t>(rank - 1.0));
    return sorted[index];
}

void print_histogram(const std::vector<std::uint64_t>& samples) {
    constexpr std::array<std::uint64_t, 12> buckets = {
        100, 250, 500, 1'000, 2'500, 5'000,
        10'000, 25'000, 50'000, 100'000, 250'000, 1'000'000};
    std::array<std::size_t, buckets.size() + 1> counts{};

    for (const std::uint64_t sample : samples) {
        const auto bucket = std::lower_bound(buckets.begin(), buckets.end(), sample);
        ++counts[static_cast<std::size_t>(std::distance(buckets.begin(), bucket))];
    }

    std::uint64_t lower = 0;
    for (std::size_t i = 0; i < buckets.size(); ++i) {
        const double percent = 100.0 * static_cast<double>(counts[i]) /
                               static_cast<double>(samples.size());
        std::cout << "    " << std::setw(8) << lower << "-" << std::setw(8)
                  << buckets[i] << " ns : " << std::setw(8) << counts[i]
                  << "  (" << std::fixed << std::setprecision(2) << percent << "%)\n";
        lower = buckets[i] + 1;
    }

    const double tail_percent = 100.0 * static_cast<double>(counts.back()) /
                                static_cast<double>(samples.size());
    std::cout << "    >" << std::setw(17) << buckets.back() << " ns : "
              << std::setw(8) << counts.back() << "  (" << std::fixed
              << std::setprecision(2) << tail_percent << "%)\n";
}

void print_latency(std::string_view name, LatencyResult result) {
    std::sort(result.nanoseconds.begin(), result.nanoseconds.end());
    long double total = 0.0;
    for (const std::uint64_t sample : result.nanoseconds) {
        total += static_cast<long double>(sample);
    }
    const long double mean = total / static_cast<long double>(result.nanoseconds.size());

    std::cout << "\n" << name << "\n"
              << "  p50 : " << percentile(result.nanoseconds, 0.50) << " ns\n"
              << "  p99 : " << percentile(result.nanoseconds, 0.99) << " ns\n"
              << "  p999: " << percentile(result.nanoseconds, 0.999) << " ns\n"
              << "  mean: " << std::fixed << std::setprecision(1)
              << static_cast<double>(mean) << " ns\n"
              << "  histogram:\n";
    print_histogram(result.nanoseconds);
    std::cout << "  checksum: " << result.checksum << "\n";
}

void print_throughput_row(std::string_view name, const ThroughputResult& result) {
    std::cout << "  " << std::left << std::setw(24) << name << std::right
              << std::setw(14) << std::fixed << std::setprecision(0)
              << result.operations_per_second << " ops/s"
              << "  " << std::setw(9) << std::setprecision(2)
              << result.seconds * 1000.0 << " ms"
              << "  checksum=" << result.checksum << "\n";
}

void print_comparison_row(std::string_view name, const ThroughputResult& optimized,
                          const ThroughputResult& reference) {
    const double speedup = optimized.operations_per_second / reference.operations_per_second;
    std::cout << "  " << std::left << std::setw(24) << name << std::right
              << std::setw(14) << std::fixed << std::setprecision(0)
              << optimized.operations_per_second
              << std::setw(14) << reference.operations_per_second
              << std::setw(12) << std::setprecision(2) << speedup << "x\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        print_usage(argv[0]);
        return 0;
    }

    Config config;
    try {
        config = parse_config(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        print_usage(argv[0]);
        return 1;
    }

    std::cout << "OrderBook benchmark harness\n"
              << "===========================\n"
              << "optimized throughput operations: " << config.orders << "\n"
              << "latency samples per workload:      " << config.latency_samples << "\n"
              << "reference comparison operations:  " << config.reference_orders << "\n\n";

    std::cout << "Optimized OrderBook throughput\n";
    print_throughput_row("limit placement", benchmark_limit_placement<OrderBook>(config.orders));
    print_throughput_row("market matching", benchmark_market_matching<OrderBook>(config.orders));
    print_throughput_row("cancellation", benchmark_cancellation<OrderBook>(config.orders));

    std::cout << "\nOptimized OrderBook latency distributions";
    print_latency("limit placement", sample_limit_latency(config.latency_samples));
    print_latency("market matching", sample_market_latency(config.latency_samples));
    print_latency("cancellation", sample_cancellation_latency(config.latency_samples));

    std::cout << "\nOptimized vs reference throughput (same workload/count)\n"
              << "  " << std::left << std::setw(24) << "workload" << std::right
              << std::setw(14) << "OrderBook" << std::setw(14) << "Reference"
              << std::setw(12) << "speedup" << "\n";

    const auto optimized_limit = benchmark_limit_placement<OrderBook>(config.reference_orders);
    const auto reference_limit = benchmark_limit_placement<ReferenceOrderBook>(config.reference_orders);
    print_comparison_row("limit placement", optimized_limit, reference_limit);

    const auto optimized_market = benchmark_market_matching<OrderBook>(config.reference_orders);
    const auto reference_market = benchmark_market_matching<ReferenceOrderBook>(config.reference_orders);
    print_comparison_row("market matching", optimized_market, reference_market);

    const auto optimized_cancel = benchmark_cancellation<OrderBook>(config.reference_orders);
    const auto reference_cancel = benchmark_cancellation<ReferenceOrderBook>(config.reference_orders);
    print_comparison_row("cancellation", optimized_cancel, reference_cancel);

    std::cout << "\nNote: throughput runs exclude setup and per-operation timing overhead.\n"
              << "Latency numbers include steady_clock measurement overhead and are machine-dependent.\n";

    return 0;
}
