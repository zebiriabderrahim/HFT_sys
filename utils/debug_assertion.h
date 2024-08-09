//
// Created by ABDERRAHIM ZEBIRI on 2024-08-08.
//

#ifndef LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H
#define LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H

#include <iostream>
#include <string_view>
#include <source_location>
#include <cstdlib>

//#ifdef DEBUG

inline void assertCondition(bool condition, std::string_view message, std::source_location loc = std::source_location::current()) noexcept {
    if (!condition) [[unlikely]] {
        std::cerr << "Assertion failed: " << message
                  << " in file " << loc.file_name()
                  << " at line " << loc.line() << std::endl;
        std::terminate();
    }
}
//TODO: Add a macro to disable assertions in release builds
//#else
//inline void assert_condition(bool, std::string_view, std::source_location = std::source_location::current()) noexcept {
//    // No-op in release builds
//}
//#endif
#endif // LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H
