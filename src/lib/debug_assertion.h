//
// Created by ABDERRAHIM ZEBIRI on 2024-08-08.
//

#ifndef LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H
#define LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H

#include <iostream>
#include <source_location>
#include <format>

/**
 * @brief Assertion function that checks a condition and prints an error message if the condition is false.
 * @param condition The condition to check.
 * @param messageFunc A function that returns the error message formatted to print if the condition is false.
 * @param loc The source location where the assertion failed.
 */
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
/**
 * @brief Assertion function that checks a condition and prints an error message if the condition is false.
 * @param condition The condition to check.
 * @param messageFunc A function that returns the error message formatted to print if the condition is false.
 * @param loc The source location where the assertion failed.
 */
#define ASSERT_CONDITION(cond, ...) assertCondition(cond, [&] { return std::format(__VA_ARGS__); })

#endif // LOW_LATENCY_TRADING_APP_DEBUG_ASSERTION_H
