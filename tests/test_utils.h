#ifndef ORDERBOOK_TEST_UTILS_H
#define ORDERBOOK_TEST_UTILS_H

#include <cstdlib>
#include <iostream>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Check failed: " #condition << " at " << __FILE__      \
                      << ':' << __LINE__ << '\n';                              \
            std::abort();                                                       \
        }                                                                       \
    } while (false)

#endif // ORDERBOOK_TEST_UTILS_H
