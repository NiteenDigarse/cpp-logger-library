#pragma once

#include <fstream>
#include <string>

#include "logger/log_sink.hpp"

namespace logging
{

    class FileSink : public LogSink
    {
    public:
        explicit FileSink(const std::string& filename);

        void write(const std::string& message) override;

        void flush() override;

    private:
        std::ofstream file_;
    };

}