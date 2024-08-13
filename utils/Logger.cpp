//
// Created by ABDERRAHIM ZEBIRI on 2024-08-12.
//

#include "Logger.h"
#include "time_utils.h"

namespace utils {
Logger::Logger(std::string_view logFilePath): logQueue_(LOG_QUEUE_SIZE) {
    logFile_.open(logFilePath.data(), std::ios::out | std::ios::app);
    assertCondition(logFile_.is_open(), "Failed to open log file.");
    logThread_ = createAndStartThread(-1,"logger {}",[this]{flushQueue();});
    assertCondition(logThread_ != nullptr, "Failed to create log thread.");

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
        for (size_t i = 0; i < BATCH_SIZE && logQueue_.size(); ++i) {
            if (auto next = logQueue_.getNextToRead()) {
                appendToBuffer(*next, buffer);
                logQueue_.updateReadIndex();
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
    } catch (const std::exception& e) {
        // Handle file writing error
        std::cerr << "Error writing to log file: " << e.what() << std::endl;
    }
}
void Logger::appendToBuffer(const LogElement& elem, std::string& buffer) {

    std::ostringstream oss;
    oss << getCurrentTimeStr() << " ";

    switch (elem.level_) {
        using enum utils::LogLevel;
    case DEBUG: oss << "[DEBUG] "; break;
    case INFO: oss << "[INFO] "; break;
    case WARNING: oss << "[WARNING] "; break;
    case ERROR: oss << "[ERROR] "; break;
    }

    std::visit([&oss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            oss << arg;
        } else {
            oss << arg;
        }
    }, elem.value);

    oss << "\n";
    buffer += oss.str();
}

} // namespace utils