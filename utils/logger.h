#ifndef LOW_LATENCY_TRADING_APP_LOGGER_H
#define LOW_LATENCY_TRADING_APP_LOGGER_H

#include <atomic>
#include <fstream>
#include <string_view>
#include <thread>
#include <memory>
#include <functional>

#include "lock_free_queue.h"
#include "thread_util.h"
#include "time_utils.h"

namespace utils {

constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

enum class LogLevel {
    DEBUG, INFO, WARNING, ERROR
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

    void log(LogLevel level, std::string_view message) noexcept {
        logQueue_.push({level, std::string(message)});
    }

    template<typename Func>
    void logWithFormat(LogLevel level, Func&& func) noexcept {
        logQueue_.push({level, std::forward<Func>(func)});
    }

  private:
    struct LogElement {
        LogLevel level;
        std::variant<std::string, std::function<std::string()>> message;
    };

    explicit Logger(std::string_view logFilePath);

    void flushQueue() noexcept;
    void writeToFile(const std::string& buffer);
    void appendToBuffer(const LogElement& elem, std::string& buffer) const ;

    std::ofstream logFile_;
    LFQueue<LogElement> logQueue_;
    std::unique_ptr<std::jthread> logThread_;
    std::atomic<bool> running_{true};
};

} // namespace utils

#define LOG_DEBUG(msg)   utils::Logger::getInstance().log(utils::LogLevel::DEBUG, msg)
#define LOG_INFO(msg)    utils::Logger::getInstance().log(utils::LogLevel::INFO, msg)
#define LOG_WARNING(msg) utils::Logger::getInstance().log(utils::LogLevel::WARNING, msg)
#define LOG_ERROR(msg)   utils::Logger::getInstance().log(utils::LogLevel::ERROR, msg)

#define LOG_DEBUGF(...)   utils::Logger::getInstance().logWithFormat(utils::LogLevel::DEBUG, [&]{ return std::format(__VA_ARGS__); })
#define LOG_INFOF(...)    utils::Logger::getInstance().logWithFormat(utils::LogLevel::INFO, [=]{ return std::format(__VA_ARGS__); })
#define LOG_WARNINGF(...) utils::Logger::getInstance().logWithFormat(utils::LogLevel::WARNING, [&]{ return std::format(__VA_ARGS__); })
#define LOG_ERRORF(...)   utils::Logger::getInstance().logWithFormat(utils::LogLevel::ERROR, [&]{ return std::format(__VA_ARGS__); })

#endif // LOW_LATENCY_TRADING_APP_LOGGER_H