#pragma once


#include <string>
#include <chrono>

#include "logger/log_level.hpp"
#include "logger/log_record.hpp"

namespace logging {

class LogFormatter {
public:
    static std::string toString(LogLevel level);

    static std::string formatTimestamp(
        std::chrono::system_clock::time_point timestamp
    );

    static std::string format(const LogRecord& record);
};

}