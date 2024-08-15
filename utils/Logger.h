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

#include "lock_free_queue.h"
#include "thread_util.h"
#include "time_utils.h"

namespace utils {

constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;
constexpr size_t BATCH_SIZE = 100;
using logTypeVariant = std::variant<char, int, long, long long, unsigned, unsigned long, unsigned long long, float, double, std::string>;

enum class LogLevel {
    DEBUG, INFO, WARNING, ERROR
};

struct LogElement {
    LogLevel level_;
    std::string timestamp_;
    logTypeVariant value;
};

class Logger {

  public:
    static Logger& getInstance(std::string_view logFilePath = "application.txt") {
        static Logger instance(logFilePath);
        return instance;
    }

    ~Logger();

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    template<typename T>
    void pushValue(LogLevel level, const T& value) noexcept {
        LogElement elem{level, utils::getCurrentTimeStr(), value};
        logQueue_.push(elem);
    }

    // Specialization for string literals
    void pushValue(LogLevel level, const char* value) noexcept {
        LogElement elem{level, utils::getCurrentTimeStr(), std::string(value)};
        logQueue_.push(elem);
    }

    template<typename... Args>
    void log(LogLevel level, Args&&... args) noexcept {
        (pushValue(level, std::forward<Args>(args)), ...);
    }

  private:
    explicit Logger(std::string_view logFilePath);

    void flushQueue() noexcept;
    void writeToFile(const std::string& buffer);
    void appendToBuffer(const LogElement& elem, std::string& buffer) const;

    std::ofstream logFile_;
    LFQueue<LogElement> logQueue_;
    std::unique_ptr<std::jthread> logThread_;
    std::atomic<bool> running_{true};
};

} // namespace utils
#define LOG_DEBUG(...)   utils::Logger::getInstance().log(utils::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...)    utils::Logger::getInstance().log(utils::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING(...) utils::Logger::getInstance().log(utils::LogLevel::WARNING, __VA_ARGS__)
#define LOG_ERROR(...)   utils::Logger::getInstance().log(utils::LogLevel::ERROR, __VA_ARGS__)

#endif // LOW_LATENCY_TRADING_APP_LOGGER_H