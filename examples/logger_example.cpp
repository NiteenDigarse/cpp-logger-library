
#include "logger/logger.hpp"
#include "logger/console_sink.hpp"
#include "logger/file_sink.hpp"

#include <iostream>
#include <memory>
#include <vector>

int main()
{
    auto console =
        std::make_shared<logging::ConsoleSink>();

    auto file =
        std::make_shared<logging::FileSink>("application.log");

    std::vector<std::shared_ptr<logging::LogSink>> sinks{
        console,
        file
    };

    logging::Logger logger(sinks, logging::LogLevel::TRACE);

    logger.trace("Trace message");
    logger.debug("Debug message");
    logger.info("Application started");
    logger.warn("Low memory warning");
    logger.error("Database connection failed");
    logger.fatal("Critical system failure");

    logger.flush();

    return 0;
}
