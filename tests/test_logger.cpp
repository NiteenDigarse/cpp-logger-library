
#include "logger/logger.hpp"

#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{

    void expect(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    // Custom test sink for capturing logger output.
    class TestSink : public logging::LogSink
    {
    public:

        void write(const std::string& message) override
        {
            std::lock_guard<std::mutex> lock(mutex_);

            messages_.push_back(message);
        }

        void flush() override
        {
            std::lock_guard<std::mutex> lock(mutex_);

            ++flush_count_;
        }

        std::vector<std::string> getMessages() const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            return messages_;
        }

        std::size_t messageCount() const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            return messages_.size();
        }

        int flushCount() const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            return flush_count_;
        }

    private:

        mutable std::mutex mutex_;

        std::vector<std::string> messages_;

        int flush_count_ = 0;
    };


    // Sink that deliberately throws exceptions.
    class ThrowingSink : public logging::LogSink
    {
    public:

        void write(const std::string&) override
        {
            throw std::runtime_error(
                "Simulated sink write failure"
            );
        }

        void flush() override
        {
            throw std::runtime_error(
                "Simulated sink flush failure"
            );
        }
    };

}

int main()
{
    using namespace logging;

    int passed = 0;
    int failed = 0;

    auto record = [&](bool result, const std::string& name)
    {
        if (result)
        {
            ++passed;
            std::cout << "[PASS] " << name << '\n';
        }
        else
        {
            ++failed;
            std::cout << "[FAIL] " << name << '\n';
        }
    };

    std::cout << "\n===== Logger Tests =====\n\n";


    // TEST 1: Valid Constructor
    try
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink});

        record(true, "Valid constructor");
    }
    catch (...)
    {
        record(false, "Valid constructor");
    }


    // TEST 2: Empty Sink Validation
    try
    {
        Logger logger({});

        record(false, "Empty sink validation");
    }
    catch (const std::invalid_argument&)
    {
        record(true, "Empty sink validation");
    }
    catch (...)
    {
        record(false, "Empty sink validation");
    }


    // TEST 3: Null Sink Validation
    try
    {
        std::shared_ptr<LogSink> null_sink;

        Logger logger({null_sink});

        record(false, "Null sink validation");
    }
    catch (const std::invalid_argument&)
    {
        record(true, "Null sink validation");
    }
    catch (...)
    {
        record(false, "Null sink validation");
    }


    // TEST 4: All Logging Methods
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink}, LogLevel::TRACE);

        logger.trace("Trace message");
        logger.debug("Debug message");
        logger.info("Info message");
        logger.warn("Warning message");
        logger.error("Error message");
        logger.fatal("Fatal message");

        auto messages = sink->getMessages();

        bool valid =
            messages.size() == 6 &&
            messages[0].find("[TRACE]") != std::string::npos &&
            messages[1].find("[DEBUG]") != std::string::npos &&
            messages[2].find("[INFO]") != std::string::npos &&
            messages[3].find("[WARN]") != std::string::npos &&
            messages[4].find("[ERROR]") != std::string::npos &&
            messages[5].find("[FATAL]") != std::string::npos;

        record(valid, "All six logging methods");
    }


    // TEST 5: Log Level Filtering
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink}, LogLevel::WARN);

        logger.trace("Trace");
        logger.debug("Debug");
        logger.info("Info");
        logger.warn("Warning");
        logger.error("Error");
        logger.fatal("Fatal");

        auto messages = sink->getMessages();

        bool valid =
            messages.size() == 3 &&
            messages[0].find("[WARN]") != std::string::npos &&
            messages[1].find("[ERROR]") != std::string::npos &&
            messages[2].find("[FATAL]") != std::string::npos;

        record(valid, "Minimum log level filtering");
    }


    // TEST 6: setLevel()
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink}, LogLevel::ERROR);

        logger.info("Ignored message");

        logger.setLevel(LogLevel::INFO);

        logger.info("Accepted message");

        auto messages = sink->getMessages();

        bool valid =
            messages.size() == 1 &&
            messages[0].find("Accepted message") != std::string::npos;

        record(valid, "Runtime log level change");
    }


    // TEST 7: getLevel()
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink}, LogLevel::DEBUG);

        bool valid = logger.getLevel() == LogLevel::DEBUG;

        logger.setLevel(LogLevel::ERROR);

        valid = valid && logger.getLevel() == LogLevel::ERROR;

        record(valid, "Get current log level");
    }


    // TEST 8: Multiple Sinks
    {
        auto first_sink = std::make_shared<TestSink>();
        auto second_sink = std::make_shared<TestSink>();

        Logger logger({first_sink, second_sink});

        logger.info("Shared message");

        auto first = first_sink->getMessages();
        auto second = second_sink->getMessages();

        bool valid =
            first.size() == 1 &&
            second.size() == 1 &&
            first[0] == second[0];

        record(valid, "Multiple sink delivery");
    }


    // TEST 9: Flush
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink});

        logger.flush();

        record(
            sink->flushCount() == 1,
            "Sink flush execution"
        );
    }


    // TEST 10: Empty Message
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink});

        logger.info("");

        auto messages = sink->getMessages();

        bool valid =
            messages.size() == 1 &&
            messages[0].find("[INFO]") != std::string::npos;

        record(valid, "Empty message handling");
    }


    // TEST 11: Multithreaded Logging
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink});

        constexpr int thread_count = 4;
        constexpr int messages_per_thread = 100;

        std::vector<std::thread> workers;

        for (int i = 0; i < thread_count; ++i)
        {
            workers.emplace_back([&logger, i]()
            {
                for (int j = 0; j < messages_per_thread; ++j)
                {
                    logger.info(
                        "Thread " + std::to_string(i) +
                        " message " + std::to_string(j)
                    );
                }
            });
        }

        for (auto& worker : workers)
        {
            worker.join();
        }

        const std::size_t expected_count =
            thread_count * messages_per_thread;

        record(
            sink->messageCount() == expected_count,
            "Multithreaded logging message count"
        );
    }


    // TEST 12: Multithreaded Message Integrity
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink});

        constexpr int thread_count = 4;
        constexpr int messages_per_thread = 100;

        std::vector<std::thread> workers;

        for (int i = 0; i < thread_count; ++i)
        {
            workers.emplace_back([&logger, i]()
            {
                for (int j = 0; j < messages_per_thread; ++j)
                {
                    logger.info(
                        "Thread " + std::to_string(i) +
                        " message " + std::to_string(j)
                    );
                }
            });
        }

        for (auto& worker : workers)
        {
            worker.join();
        }

        auto messages = sink->getMessages();

        bool valid = messages.size() == 400;

        for (const auto& message : messages)
        {
            if (message.find("[INFO]") == std::string::npos ||
                message.find("Thread ") == std::string::npos)
            {
                valid = false;
                break;
            }
        }

        record(valid, "Multithreaded message integrity");
    }


    // TEST 13: Invalid Constructor LogLevel
    try
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger(
            {sink},
            static_cast<LogLevel>(255)
        );

        record(false, "Invalid constructor log level");
    }
    catch (const std::invalid_argument&)
    {
        record(true, "Invalid constructor log level");
    }
    catch (...)
    {
        record(false, "Invalid constructor log level");
    }


    // TEST 14: Invalid setLevel()
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink}, LogLevel::INFO);

        bool exception_thrown = false;

        try
        {
            logger.setLevel(static_cast<LogLevel>(255));
        }
        catch (const std::invalid_argument&)
        {
            exception_thrown = true;
        }
        catch (...)
        {
        }

        record(
            exception_thrown,
            "Invalid setLevel validation"
        );
    }


    // TEST 15: Invalid log() Severity
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink});

        bool exception_thrown = false;

        try
        {
            logger.log(
                static_cast<LogLevel>(255),
                "Invalid severity message"
            );
        }
        catch (const std::invalid_argument&)
        {
            exception_thrown = true;
        }
        catch (...)
        {
        }

        bool valid =
            exception_thrown &&
            sink->messageCount() == 0;

        record(
            valid,
            "Invalid log severity validation"
        );
    }


    // TEST 16: Previous Level Preserved After Invalid setLevel()
    {
        auto sink = std::make_shared<TestSink>();

        Logger logger({sink}, LogLevel::INFO);

        bool exception_thrown = false;

        try
        {
            logger.setLevel(static_cast<LogLevel>(255));
        }
        catch (const std::invalid_argument&)
        {
            exception_thrown = true;
        }
        catch (...)
        {
        }

        bool valid =
            exception_thrown &&
            logger.getLevel() == LogLevel::INFO;

        record(
            valid,
            "Previous log level preserved after invalid setLevel"
        );
    }


    // TEST 17: Sink Write Exception Propagation
    {
        auto throwing_sink = std::make_shared<ThrowingSink>();
        auto second_sink = std::make_shared<TestSink>();

        Logger logger({throwing_sink, second_sink});

        bool exception_thrown = false;

        try
        {
            logger.info("Test write exception");
        }
        catch (const std::runtime_error&)
        {
            exception_thrown = true;
        }
        catch (...)
        {
        }

        // The second sink must not receive the message.
        // The logger mutex must be released during stack unwinding.
        bool valid =
            exception_thrown &&
            second_sink->messageCount() == 0 &&
            logger.getLevel() == LogLevel::TRACE;

        record(
            valid,
            "Sink write exception propagation and mutex release"
        );
    }


    // TEST 18: Sink Flush Exception Propagation
    {
        auto throwing_sink = std::make_shared<ThrowingSink>();
        auto second_sink = std::make_shared<TestSink>();

        Logger logger({throwing_sink, second_sink});

        bool exception_thrown = false;

        try
        {
            logger.flush();
        }
        catch (const std::runtime_error&)
        {
            exception_thrown = true;
        }
        catch (...)
        {
        }

        // The second sink must not be flushed.
        // getLevel() checks that the mutex was released.
        bool valid =
            exception_thrown &&
            second_sink->flushCount() == 0 &&
            logger.getLevel() == LogLevel::TRACE;

        record(
            valid,
            "Sink flush exception propagation and mutex release"
        );
    }


    // TEST SUMMARY
    std::cout << "\n===== Logger Test Summary =====\n";

    std::cout << "Passed: " << passed << '\n';
    std::cout << "Failed: " << failed << '\n';

    if (failed == 0)
    {
        std::cout << "\nAll Logger tests passed!\n";
    }
    else
    {
        std::cout << "\nSome Logger tests failed!\n";
    }

    return failed == 0 ? 0 : 1;
}
