
#include "logger/log_formatter.hpp"
#include "logger/log_record.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <thread>

void expect(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main()
{
    using namespace logging;

    try
    {
        LogRecord record{
            "Application started",
            LogLevel::INFO,
            std::chrono::system_clock::from_time_t(0),
            std::this_thread::get_id()
        };

        const std::string result = LogFormatter::format(record);

        expect(result.find("[INFO]") != std::string::npos,
               "Log level missing");

        expect(result.find("Application started") != std::string::npos,
               "Message missing");

        expect(result.find("[Thread: ") != std::string::npos,
               "Thread ID missing");

        const std::string timestamp =
            LogFormatter::formatTimestamp(record.timestamp);

        expect(timestamp.size() == 19,
               "Timestamp format length incorrect");

        expect(timestamp[4] == '-' &&
               timestamp[7] == '-' &&
               timestamp[10] == ' ' &&
               timestamp[13] == ':' &&
               timestamp[16] == ':',
               "Timestamp format incorrect");

        std::cout << "Formatted output:\n";
        std::cout << result << '\n';

        std::cout << "All LogFormatter tests passed!\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }
}
