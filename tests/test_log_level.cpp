
#include "logger/log_formatter.hpp"
#include "logger/log_level.hpp"

#include <iostream>
#include <stdexcept>

void expect(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main()
{
    using logging::LogLevel;
    using logging::LogFormatter;

    try
    {
        expect(LogFormatter::toString(LogLevel::TRACE) == "TRACE",
               "TRACE conversion failed");

        expect(LogFormatter::toString(LogLevel::DEBUG) == "DEBUG",
               "DEBUG conversion failed");

        expect(LogFormatter::toString(LogLevel::INFO) == "INFO",
               "INFO conversion failed");

        expect(LogFormatter::toString(LogLevel::WARN) == "WARN",
               "WARN conversion failed");

        expect(LogFormatter::toString(LogLevel::ERROR) == "ERROR",
               "ERROR conversion failed");

        expect(LogFormatter::toString(LogLevel::FATAL) == "FATAL",
               "FATAL conversion failed");

        expect(LogFormatter::toString(static_cast<LogLevel>(255)) == "UNKNOWN",
               "Unknown level conversion failed");

        std::cout << "All LogLevel tests passed!\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }
}
