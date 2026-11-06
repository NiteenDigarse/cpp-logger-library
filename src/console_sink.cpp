#include "logger/console_sink.hpp"

#include <iostream>

namespace logging
{

    void ConsoleSink::write(const std::string& message)
    {
        std::cout << message << '\n';
    }

    void ConsoleSink::flush()
    {
        std::cout.flush();
    }

}