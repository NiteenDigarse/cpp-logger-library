#pragma once

#include <string>

#include "logger/log_sink.hpp"

namespace logging
{

    class ConsoleSink : public LogSink
    {
    public:
        void write(const std::string& message) override;

        void flush() override;
    };

}