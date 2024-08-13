//
// Created by ABDERRAHIM ZEBIRI on 2024-08-12.
//

#ifndef LOW_LATENCY_TRADING_APP_LOGGER_H
#define LOW_LATENCY_TRADING_APP_LOGGER_H

#include <fstream>
#include <string>
#include <string_view>
#include <variant>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

#include "thread_util.h"
#include "lock_free_queue.h"

namespace utils {

constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;
constexpr size_t BATCH_SIZE = 100;

enum class LogType {
    CHAR, INT, LONG, LONG_LONG, UNSIGNED, UNSIGNED_LONG, UNSIGNED_LONG_LONG, FLOAT, DOUBLE, STRING
};

enum class LogLevel {
    DEBUG, INFO, WARNING, ERROR
};

struct LogElement {
    LogType type_;
    LogLevel level_;
    std::chrono::system_clock::time_point timestamp_;
    std::variant<char, int, long, long long, unsigned, unsigned long, unsigned long long, float, double, std::string> value;
};

class Logger {
  public:
    explicit Logger(std::string_view logFilePath);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    template<typename T>
    void pushValue(LogLevel level, const T& value) noexcept {
        LogElement elem{getLogType<T>(), level, std::chrono::system_clock::now(), value};
        *(logQueue_.getNextToWrite()) = elem;
    }

    template<typename... Args>
    void log(LogLevel level, Args&&... args) noexcept {
        (pushValue(level, std::forward<Args>(args)), ...);
    }

  private:
    void flushQueue() noexcept;
    void writeToFile(const std::string& buffer);
    void appendToBuffer(const LogElement& elem, std::string& buffer);

    template<typename T>
    static constexpr LogType getLogType() {
        using enum utils::LogType;
        if constexpr (std::is_same_v<T, char>) return CHAR;
        else if constexpr (std::is_same_v<T, int>) return INT;
        else if constexpr (std::is_same_v<T, long>) return LONG;
        else if constexpr (std::is_same_v<T, long long>) return LONG_LONG;
        else if constexpr (std::is_same_v<T, unsigned>) return UNSIGNED;
        else if constexpr (std::is_same_v<T, unsigned long>) return UNSIGNED_LONG;
        else if constexpr (std::is_same_v<T, unsigned long long>) return UNSIGNED_LONG_LONG;
        else if constexpr (std::is_same_v<T, float>) return FLOAT;
        else if constexpr (std::is_same_v<T, double>) return DOUBLE;
        else if constexpr (std::is_same_v<T, std::string>) return STRING;
        else static_assert(false, "Unsupported type for logging.");
    }

    std::ofstream logFile_;
    LFQueue<LogElement> logQueue_;
    std::unique_ptr<std::jthread> logThread_;
    std::atomic<bool> running_{true};
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_LOGGER_H

