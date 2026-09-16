cmake_minimum_required(VERSION 3.14)
project(OrderBook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Main library
add_library(orderbook_lib
    src/order_book.cpp
    src/price_level.cpp
)

target_include_directories(orderbook_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Tests
enable_testing()
add_executable(test_order_book tests/test_order_book.cpp)
target_link_libraries(test_order_book PRIVATE orderbook_lib)

add_executable(test_price_level tests/test_price_level.cpp)
target_link_libraries(test_price_level PRIVATE orderbook_lib)

add_executable(test_matching tests/test_matching.cpp)
target_link_libraries(test_matching PRIVATE orderbook_lib)

add_test(NAME test_order_book COMMAND test_order_book)
add_test(NAME test_price_level COMMAND test_price_level)
add_test(NAME test_matching COMMAND test_matching)

# Benchmarks
add_executable(benchmark_order_book benchmarks/benchmark_order_book.cpp)
target_link_libraries(benchmark_order_book PRIVATE orderbook_lib)
