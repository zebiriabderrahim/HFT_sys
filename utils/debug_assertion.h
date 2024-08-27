//
// Created by ABDERRAHIM ZEBIRI on 2024-08-08.
//

#ifndef LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H
#define LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H

#include <iostream>
#include <string_view>
#include <source_location>
#include <cstdlib>
#include <format>
#include <functional>

//#ifdef DEBUG

template <typename Func>
inline void assertCondition(bool condition, Func&& messageFunc, std::source_location loc = std::source_location::current()) noexcept {
    if (!condition) [[unlikely]] {
        std::cerr << "Assertion failed: " << messageFunc() << std::endl
                  << " in file " << loc.file_name()
                  << " at line " << loc.line() << std::endl
                  << " function " << loc.function_name() << std::endl;
        std::terminate();
    }
}
#define ASSERT_CONDITION(cond, ...) assertCondition(cond, [&] { return std::format(__VA_ARGS__); })


 //TODO: Add a macro to disable assertions in release builds
//#else
//inline void assert_condition(bool, std::string_view, std::source_location = std::source_location::current()) noexcept {
//    // No-op in release builds
//}
//#endif
#endif // LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H
