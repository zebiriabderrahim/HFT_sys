//
// Created by ABDERRAHIM ZEBIRI on 2024-08-11.
//

#ifndef LOW_LATENCY_TRADING_APP_TIME_UTILS_H
#define LOW_LATENCY_TRADING_APP_TIME_UTILS_H

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>


namespace utils {

using Nanos = int64_t;

constexpr Nanos NANOS_TO_MICROS = 1000;
constexpr Nanos MICROS_TO_MILLIS = 1000;
constexpr Nanos MILLIS_TO_SECS = 1000;
constexpr Nanos NANOS_TO_MILLIS = NANOS_TO_MICROS * MICROS_TO_MILLIS;
constexpr Nanos NANOS_TO_SECS = NANOS_TO_MILLIS * MILLIS_TO_SECS;

inline auto getCurrentNanos() noexcept -> Nanos {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline auto getCurrentTimeStr() -> std::string {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time, &tm); // Thread-safe version of localtime
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_TIME_UTILS_H
