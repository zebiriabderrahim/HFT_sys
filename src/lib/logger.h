#ifndef LOW_LATENCY_TRADING_APP_LOGGER_H
#define LOW_LATENCY_TRADING_APP_LOGGER_H

#include <atomic>
#include <fstream>
#include <string_view>
#include <thread>
#include <memory>
#include <any>

#include "lock_free_queue.h"

/**
 * @file logger.h
 * @brief Defines a low-latency, thread-safe logging system for a trading application.
 */

namespace utils {

constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

/**
 * @brief Enumeration of log levels.
 */
enum class LogLevel {
    DEBUG,   ///< Detailed information, typically of interest only when diagnosing problems.
    INFO,    ///< Confirmation that things are working as expected.
    WARNING, ///< An indication that something unexpected happened, or indicative of some problem in the near future.
    ERROR    ///< Due to a more serious problem, the software has not been able to perform some function.
};

/**
 * @class Logger
 * @brief A singleton class providing thread-safe, low-latency logging functionality.
 *
 * This logger uses a lock-free queue to store log messages, which are then processed
 * and written to a file by a separate thread. This design minimizes the impact of
 * logging on the main application thread, making it suitable for low-latency environments.
 */
class Logger {
  public:
    /**
     * @brief Get the singleton instance of the Logger.
     * @param logFilePath The path to the log file. Defaults to "application.log".
     * @return A reference to the Logger instance.
     */
    static Logger& getInstance(std::string_view logFilePath = "application.log");

    /**
     * @brief Destructor. Ensures all logs are flushed and resources are properly released.
     */
    ~Logger();

    // Delete copy and move constructors and assignment operators
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Log a message with the specified log level and format.
     *
     * This method is variadic and type-safe. It accepts a format string and any number of arguments,
     * similar to std::format. The formatting is deferred until the log is actually written to file.
     *
     * @param level The log level of the message.
     * @param format_string The format string for the log message.
     * @param args The arguments to be formatted into the log message.
     */
    template <typename... Args>
    void log(LogLevel level, std::string_view format_string, Args&&... args) noexcept {
        LogElement elem{level, format_string};
        elem.args.reserve(sizeof...(Args));
        (elem.args.emplace_back(std::forward<Args>(args)), ...);
        logQueue_.push(elem);
    }

  private:
    /**
     * @struct LogElement
     * @brief Represents a single log entry in the queue.
     */
    struct LogElement {
        LogLevel level;                 ///< The log level of the message.
        std::string_view formatString; ///< The format string for the log message.
        std::vector<std::any> args;     ///< The arguments to be formatted into the log message.
    };

    /**
     * @brief Constructor. Initializes the logger with the specified log file.
     * @param logFilePath The path to the log file.
     */
    explicit Logger(std::string_view logFilePath);

    /**
     * @brief Flushes the log queue, writing pending logs to the file.
     */
    auto flushQueue() noexcept -> void;

    /**
     * @brief Writes the given buffer to the log file.
     * @param buffer The string buffer to write to the file.
     */
    auto writeToFile(const std::string& buffer) -> void;

    /**
     * @brief Appends a formatted log message to the given buffer.
     * @param elem The LogElement containing the log information.
     * @param buffer The buffer to append the formatted log message to.
     */
    auto appendToBuffer(const LogElement& elem, std::string& buffer) const -> void;

    /**
     * @brief Converts an std::any object to a string representation.
     * @param arg The std::any object to convert.
     * @return A string representation of the argument.
     */
    auto anyToString(const std::any &arg) const -> std::string;

    /**
     * @brief Formats a message using the given format string and arguments.
     * @param format_string The format string.
     * @param args The vector of arguments as std::any.
     * @return The formatted string.
     */
    auto formatMessage(std::string_view format_string, const std::vector<std::any> &args) const -> std::string;

    std::ofstream logFile_;                    ///< The output file stream for writing logs.
    LFQueue<LogElement> logQueue_;             ///< The lock-free queue for storing log messages.
    std::unique_ptr<std::jthread> logThread_;  ///< The thread responsible for processing the log queue.
    std::atomic<bool> running_{true};          ///< Flag indicating whether the logger is running.
    std::vector<std::any> argVector_;          ///< Temporary vector for storing arguments during logging.
};

} // namespace lib

/**
 * @def LOG_INFO(msg, ...)
 * @brief Macro for logging an INFO level message.
 * @param msg The format string for the log message.
 * @param ... The arguments to be formatted into the log message.
 */
#define LOG_INFO(msg, ...) utils::Logger::getInstance().log(utils::LogLevel::INFO, msg, ##__VA_ARGS__)

/**
 * @def LOG_DEBUG(msg, ...)
 * @brief Macro for logging a DEBUG level message.
 * @param msg The format string for the log message.
 * @param ... The arguments to be formatted into the log message.
 */
#define LOG_DEBUG(msg, ...) utils::Logger::getInstance().log(utils::LogLevel::DEBUG, msg, ##__VA_ARGS__)

/**
 * @def LOG_WARNING(msg, ...)
 * @brief Macro for logging a WARNING level message.
 * @param msg The format string for the log message.
 * @param ... The arguments to be formatted into the log message.
 */
#define LOG_WARNING(msg, ...) utils::Logger::getInstance().log(utils::LogLevel::WARNING, msg, ##__VA_ARGS__)

/**
 * @def LOG_ERROR(msg, ...)
 * @brief Macro for logging an ERROR level message.
 * @param msg The format string for the log message.
 * @param ... The arguments to be formatted into the log message.
 */
#define LOG_ERROR(msg, ...) utils::Logger::getInstance().log(utils::LogLevel::ERROR, msg, ##__VA_ARGS__)

#endif // LOW_LATENCY_TRADING_APP_LOGGER_H