#include "logger/file_sink.hpp"

#include <stdexcept>

namespace logging
{

    FileSink::FileSink(const std::string& filename)
    {
        file_.open(filename, std::ios::out | std::ios::app);

        if (!file_.is_open())
        {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }

    void FileSink::write(const std::string& message)
    {
        file_ << message << '\n';

        if (!file_)
        {
            throw std::runtime_error("Failed to write to log file");
        }
    }

    void FileSink::flush()
    {
        file_.flush();

        if (!file_)
        {
            throw std::runtime_error("Failed to flush log file");
        }
    }

}