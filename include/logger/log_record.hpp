#pragma once

#include <string>
#include <chrono>
#include <thread>

#include "logger/log_level.hpp"

namespace logging
{

    struct LogRecord
    {
        std::string message;
        LogLevel level;
        std::chrono::system_clock::time_point timestamp;
        std::thread::id thread_id;
    };

}