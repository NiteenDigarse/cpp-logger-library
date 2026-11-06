#include "logger/log_formatter.hpp"

#include<chrono>
#include<ctime>
#include<iomanip>
#include<sstream>

namespace logging
{
//tostring()
    std::string LogFormatter::toString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::TRACE:
            return "TRACE";

        case LogLevel::INFO:
            return "INFO";

        case LogLevel::DEBUG:
            return "DEBUG";

        case LogLevel::WARN:
            return "WARN";

        case LogLevel::ERROR:
            return "ERROR";

        case LogLevel::FATAL:
            return "FATAL";

        default:
            return "UNKNOWN";
        }
    }
// formateTimestamp()
std::string LogFormatter::formatTimestamp(
    std::chrono::system_clock::time_point timestamp)
{
    // Step 1: Convert time_point into time_t
    std::time_t time =
        std::chrono::system_clock::to_time_t(timestamp);

    // Step 2: Create tm structure
    std::tm tm{};

    // Step 3: Convert time_t into local calendar time
    #if defined(_WIN32)
        localtime_s(&tm, &time);
    #else
        localtime_r(&time, &tm);
    #endif

    // Step 4: Create string stream
    std::ostringstream oss;

    // Step 5: Format date and time
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // Step 6: Return formatted string
    return oss.str();
}

// formate()

std:: string LogFormatter::format(const LogRecord&record)
{

    std::ostringstream oss;

    // Step 1: Timestamp
    oss << "["
        << LogFormatter::formatTimestamp(record.timestamp)
        << "] ";

    // Step 2: Log Level
    oss << "["
        << LogFormatter::toString(record.level)
        << "] ";

    // Step 3: Thread ID
    oss << "[Thread: "
        << record.thread_id
        << "] ";

    // Step 4: Message
    oss << record.message;

    // Step 5: Return formatted string
    return oss.str();

}
}
