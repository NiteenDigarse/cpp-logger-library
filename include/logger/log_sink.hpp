#pragma once

#include <string>

namespace logging {

class LogSink {
public:
    virtual ~LogSink() = default;

    virtual void write(const std::string& message) = 0;

    virtual void flush() = 0;
};

}