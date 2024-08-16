//
// Created by ABDERRAHIM ZEBIRI on 2024-08-12.
//
#ifndef LOW_LATENCY_TRADING_APP_LOGGER_H
#define LOW_LATENCY_TRADING_APP_LOGGER_H

#include <fstream>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>

#if __cplusplus >= 202002L
#include <format>
#endif

#include "lock_free_queue.h"
#include "thread_util.h"
#include "time_utils.h"

namespace utils {

constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

enum class LogLevel {
    DEBUG, INFO, WARNING, ERROR
};

struct LogElement {
    LogLevel level;
    std::string message;
};

class Logger {
  public:
    static Logger& getInstance(std::string_view logFilePath = "application.log") {
        static Logger instance(logFilePath);
        return instance;
    }

    ~Logger();

    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    template<typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args) noexcept {
        logQueue_.push(LogElement{level, std::move(formatMessage(fmt, std::forward<Args>(args)...))});
    }

  private:
    explicit Logger(std::string_view logFilePath);

    void flushQueue() noexcept;
    void writeToFile(const std::string& buffer);
    void appendToBuffer(const LogElement& elem, std::string& buffer) const;

    template<typename... Args>
    std::string formatMessage(std::string_view fmt, Args&&... args) const {
        try {
            return std::vformat(fmt, std::make_format_args(args...));
        } catch (const std::format_error& e) {
            return "Format error: " + std::string(e.what());
        }
    }

    std::ofstream logFile_;
    LFQueue<LogElement> logQueue_;
    std::unique_ptr<std::jthread> logThread_;
    std::atomic<bool> running_{true};
};

} // namespace utils

#define LOG_DEBUG(fmt, ...)   utils::Logger::getInstance().log(utils::LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    utils::Logger::getInstance().log(utils::LogLevel::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) utils::Logger::getInstance().log(utils::LogLevel::WARNING, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   utils::Logger::getInstance().log(utils::LogLevel::ERROR, fmt, ##__VA_ARGS__)

#endif // LOW_LATENCY_TRADING_APP_LOGGER_H