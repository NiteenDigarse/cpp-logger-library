
#include "logger/logger.hpp"

#include "logger/log_formatter.hpp"
#include "logger/log_record.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

namespace logging
{

    namespace
    {

        bool isValidLogLevel(LogLevel level)
        {
            switch (level)
            {
                case LogLevel::TRACE:
                case LogLevel::DEBUG:
                case LogLevel::INFO:
                case LogLevel::WARN:
                case LogLevel::ERROR:
                case LogLevel::FATAL:
                    return true;

                default:
                    return false;
            }
        }

        void validateLogLevel(LogLevel level)
        {
            if (!isValidLogLevel(level))
            {
                throw std::invalid_argument(
                    "Invalid log level"
                );
            }
        }

    }

    Logger::Logger(
        std::vector<std::shared_ptr<LogSink>> sinks,
        LogLevel min_level
    )
        : sinks_(std::move(sinks)),
          min_level_(min_level)
    {
        validateLogLevel(min_level_);

        if (sinks_.empty())
        {
            throw std::invalid_argument(
                "Logger requires at least one sink"
            );
        }

        for (const auto& sink : sinks_)
        {
            if (!sink)
            {
                throw std::invalid_argument(
                    "Logger cannot contain a null sink"
                );
            }
        }
    }

    void Logger::setLevel(LogLevel level)
    {
        validateLogLevel(level);

        std::lock_guard<std::mutex> lock(mutex_);

        min_level_ = level;
    }

    LogLevel Logger::getLevel() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        return min_level_;
    }

    void Logger::log(
        LogLevel level,
        const std::string& message
    )
    {
        validateLogLevel(level);

        std::lock_guard<std::mutex> lock(mutex_);

        if (level < min_level_)
        {
            return;
        }

        LogRecord record{
            message,
            level,
            std::chrono::system_clock::now(),
            std::this_thread::get_id()
        };

        const std::string formatted_message =
            LogFormatter::format(record);

        // Sink exceptions propagate to the caller.
        // If one sink throws, subsequent sinks are not called.
        for (const auto& sink : sinks_)
        {
            sink->write(formatted_message);
        }
    }

    void Logger::trace(const std::string& message)
    {
        log(LogLevel::TRACE, message);
    }

    void Logger::debug(const std::string& message)
    {
        log(LogLevel::DEBUG, message);
    }

    void Logger::info(const std::string& message)
    {
        log(LogLevel::INFO, message);
    }

    void Logger::warn(const std::string& message)
    {
        log(LogLevel::WARN, message);
    }

    void Logger::error(const std::string& message)
    {
        log(LogLevel::ERROR, message);
    }

    void Logger::fatal(const std::string& message)
    {
        log(LogLevel::FATAL, message);
    }

    void Logger::flush()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Flush exceptions propagate to the caller.
        // If one sink throws, subsequent sinks are not flushed.
        for (const auto& sink : sinks_)
        {
            sink->flush();
        }
    }

}
