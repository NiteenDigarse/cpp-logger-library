#pragma once

#include <cstdint>

namespace logging {

enum class LogLevel : std::uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
};

}