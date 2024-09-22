#include <gtest/gtest.h>
#include <fstream>
#include <regex>
#include <thread>

#include "lib/logger.h"

using namespace utils;

class LoggerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Use a unique log file for each test
        logFileName = std::format("test_log_{}.log" , std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
         Logger::setLogFile(logFileName);
    }

    void TearDown() override {
        // Clean up the log file after each test
        std::remove(logFileName.c_str());
    }

    std::string logFileName;

    // Helper function to read the entire log file
    [[nodiscard]] std::string readLogFile() const {
        std::ifstream file(logFileName);
        return std::string{(std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()};
    }
};

TEST_F(LoggerTest, LogLevels) {
    [[maybe_unused]] auto const& logger = Logger::getInstance(logFileName);

    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARNING("Warning message");
    LOG_ERROR("Error message");

    // Give some time for the logger thread to process
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::string logContent = readLogFile();

    EXPECT_TRUE(logContent.find("[DEBUG] Debug message") != std::string::npos);
    EXPECT_TRUE(logContent.find("[INFO] Info message") != std::string::npos);
    EXPECT_TRUE(logContent.find("[WARNING] Warning message") != std::string::npos);
    EXPECT_TRUE(logContent.find("[ERROR] Error message") != std::string::npos);
}

TEST_F(LoggerTest, FormatString) {
    [[maybe_unused]] auto const& logger = Logger::getInstance(logFileName);


    LOG_INFO("Formatted message: {} {}", "Hello", "World");

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::string logContent = readLogFile();

    EXPECT_TRUE(logContent.find("Formatted message: Hello World") != std::string::npos);
}

TEST_F(LoggerTest, MultipleArguments) {
    [[maybe_unused]] auto const& logger = Logger::getInstance(logFileName);

    LOG_INFO("Multiple types: {} {} {} {}", 42, 3.14, true, "A");

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::string logContent = readLogFile();

    EXPECT_TRUE(logContent.find("Multiple types: 42 3.140000 true A") != std::string::npos);
}

TEST_F(LoggerTest, SingleProducerLogging) {
    [[maybe_unused]] auto const& logger = Logger::getInstance(logFileName);

    // Simulate a single producer thread
    for (int i = 0; i < 4; ++i) {
        LOG_INFO("Message {} from iteration", i);
    }

    // Wait for the logger to process all messages
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::string logContent = readLogFile();

    for (int i = 0; i < 4; ++i) {
        std::string expectedMessage = std::format("Message {} from iteration", i);
        EXPECT_TRUE(logContent.find(expectedMessage) != std::string::npos)
            << "Expected message not found: " << expectedMessage;

    }
}

TEST_F(LoggerTest, TimestampFormat) {
    [[maybe_unused]] auto const& logger = Logger::getInstance(logFileName);

    LOG_INFO("Test message");

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::string logContent = readLogFile();

    // Regex to match ISO 8601 timestamp format
    std::regex logEntryRegex(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] \[INFO\] Test message\n)");

    EXPECT_TRUE(std::regex_search(logContent, logEntryRegex));

}

TEST_F(LoggerTest, LargeNumberOfLogs) {
    [[maybe_unused]] auto const& logger = Logger::getInstance(logFileName);

    for (int i = 0; i < 3000; ++i) {
        LOG_INFO("Log message {}", i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::string logContent = readLogFile();

    EXPECT_EQ(std::count(logContent.begin(), logContent.end(), '\n'), 3000);
}
