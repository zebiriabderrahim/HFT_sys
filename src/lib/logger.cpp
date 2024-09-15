#include "logger.h"
#include "time_utils.h"
#include "thread_util.h"

#include <format>
#include <typeindex>
#include <unordered_map>



namespace utils {

Logger::Logger(std::string_view logFilePath): logQueue_(LOG_QUEUE_SIZE) {
    logFile_.open(logFilePath.data(), std::ios::out | std::ios::app);
    ASSERT_CONDITION(logFile_.is_open(), "Failed to open log file: {}", logFilePath);
    logThread_ = createAndStartThread(-1, "logger {}", [this]{flushQueue();});
    ASSERT_CONDITION(logThread_ != nullptr, "Failed to create logger thread");
}

Logger::~Logger() {
    running_ = false;
    if (logThread_ && logThread_->joinable()) {
        logThread_->join();
    }
    logFile_.close();
}

void Logger::flushQueue() noexcept {
    std::string buffer;
    while (running_.load(std::memory_order_acquire)) {
        buffer.clear();
        for (size_t i = 0; i < 100 && logQueue_.size(); ++i) {
            if (auto next = logQueue_.pop()) {
                appendToBuffer(*next, buffer);
            }
        }
        if (!buffer.empty()) {
            writeToFile(buffer);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Logger::writeToFile(const std::string& buffer) {
    try {
        logFile_ << buffer;
        logFile_.flush();

        if (!logFile_ || logFile_.bad()) {
            throw std::ios_base::failure("Stream error");
        }
    } catch (const std::ios_base::failure& e) {
        std::cerr << "I/O error writing to log file: " << e.what() << std::endl;
    } catch (const std::system_error& e) {
        std::cerr << "System error writing to log file: " << e.what()
                  << " (error code: " << e.code() << ")" << std::endl;
    }
}

void Logger::appendToBuffer(const LogElement& elem, std::string& buffer) const {
    const char* logLevel = [&]() {
        switch (elem.level) {
            using enum utils::LogLevel;
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }();

    buffer.append(std::format("[{}] [{}] ", getCurrentTimeStr(), logLevel));

    // Format the message using the stored format string and arguments
    try {
        std::string formatted_message = formatMessage(elem.formatString, elem.args);
        buffer.append(formatted_message);
    } catch (const std::exception& e) {
        buffer.append("Error formatting log message: ");
        buffer.append(e.what());
    }

    buffer.push_back('\n');
}
std::string Logger::formatMessage(std::string_view format_string, const std::vector<std::any>& args) const {
    std::string result(format_string);
    size_t arg_index = 0;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == '{' && i + 1 < result.size() && result[i + 1] == '}' && arg_index < args.size()) {
            std::string arg_str = anyToString(args[arg_index]);
            result.replace(i, 2, arg_str);
            i += arg_str.length() - 1;
            ++arg_index;
        }
    }
    return result;
}

std::string Logger::anyToString(const std::any& arg) const {
    if (arg.type() == typeid(int)) {
        return std::to_string(std::any_cast<int>(arg));
    } else if (arg.type() == typeid(std::size_t)) {
        return std::to_string(std::any_cast<std::size_t>(arg));
    } else if (arg.type() == typeid(double)) {
        return std::to_string(std::any_cast<double>(arg));
    } else if (arg.type() == typeid(long)) {
        return std::to_string(std::any_cast<long>(arg));
    }else if (arg.type() == typeid(float)) {
        return std::to_string(std::any_cast<float>(arg));
    } else if (arg.type() == typeid(char)) {
        return std::string{1, std::any_cast<char>(arg)};
    } else if (arg.type() == typeid(bool)) {
        return std::any_cast<bool>(arg) ? "true" : "false";
    } else if (arg.type() == typeid(std::string)) {
        return std::any_cast<std::string>(arg);
    } else if (arg.type() == typeid(const char*)) {
        return std::string{std::any_cast<const char*>(arg)};
    } else if(arg.type() ==typeid(Nanos)){
        return convertNanosToTimeStr(std::any_cast<Nanos>(arg));
    }

    return "Unsupported type: " + std::string(arg.type().name());
}
Logger &Logger::getInstance(std::string_view logFilePath) {
        static Logger instance(logFilePath);
        return instance;
}

} // namespace lib