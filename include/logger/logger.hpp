
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "logger/log_level.hpp"
#include "logger/log_sink.hpp"

namespace logging
{

    class Logger
    {
    public:

        explicit Logger(
            std::vector<std::shared_ptr<LogSink>> sinks,
            LogLevel min_level = LogLevel::TRACE
        );

        // Logger is intentionally non-copyable and non-movable.
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        void setLevel(LogLevel level);

        LogLevel getLevel() const;

        void log(
            LogLevel level,
            const std::string& message
        );

        void trace(const std::string& message);
        void debug(const std::string& message);
        void info(const std::string& message);
        void warn(const std::string& message);
        void error(const std::string& message);
        void fatal(const std::string& message);

        void flush();

    private:

        std::vector<std::shared_ptr<LogSink>> sinks_;

        LogLevel min_level_;

        mutable std::mutex mutex_;
    };

}
