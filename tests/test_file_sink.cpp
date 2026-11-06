
#include "logger/file_sink.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <exception>

namespace fs = std::filesystem;

bool check(bool condition, const std::string& test_name)
{
    if (condition)
    {
        std::cout << "[PASS] " << test_name << '\n';
        return true;
    }

    std::cout << "[FAIL] " << test_name << '\n';
    return false;
}

std::vector<std::string> readLines(const std::string& filename)
{
    std::ifstream file(filename);

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line))
    {
        lines.push_back(line);
    }

    return lines;
}

int main()
{
    const std::string filename = "test_application.log";

    int passed = 0;
    int failed = 0;

    auto record = [&](bool result)
    {
        result ? ++passed : ++failed;
    };

    std::cout << "\n===== FileSink Tests =====\n\n";

    // Clean previous test file
    fs::remove(filename);

    // TEST 1: File Creation
    try
    {
        logging::FileSink sink(filename);

        record(check(
            fs::exists(filename),
            "File creation"
        ));
    }
    catch (const std::exception& e)
    {
        record(check(false, "File creation"));
        std::cout << "Error: " << e.what() << '\n';
    }

    // TEST 2: Message Writing
    try
    {
        logging::FileSink sink(filename);

        sink.write("Application started");
        sink.write("Database connected");
        sink.write("Logger is working");

        sink.flush();
    }
    catch (const std::exception& e)
    {
        std::cout << "Write error: " << e.what() << '\n';
    }

    auto lines = readLines(filename);

    record(check(
        lines.size() == 3 &&
        lines[0] == "Application started" &&
        lines[1] == "Database connected" &&
        lines[2] == "Logger is working",
        "Message writing and content verification"
    ));

    // TEST 3: Append Behaviour
    try
    {
        logging::FileSink sink(filename);

        sink.write("New application session");
        sink.flush();
    }
    catch (const std::exception& e)
    {
        std::cout << "Append error: " << e.what() << '\n';
    }

    lines = readLines(filename);

    record(check(
        lines.size() == 4 &&
        lines[0] == "Application started" &&
        lines[3] == "New application session",
        "Append preserves existing logs"
    ));

    // TEST 4: Flush Behaviour
    try
    {
        logging::FileSink sink(filename);

        sink.write("Flush test message");
        sink.flush();

        auto flushed_lines = readLines(filename);

        record(check(
            !flushed_lines.empty() &&
            flushed_lines.back() == "Flush test message",
            "Flush writes buffered output"
        ));
    }
    catch (const std::exception& e)
    {
        record(check(false, "Flush behaviour"));
        std::cout << "Flush error: " << e.what() << '\n';
    }

    // TEST 5: Invalid Path Error Handling
    try
    {
        logging::FileSink sink(
            "non_existing_directory/test.log"
        );

        record(check(
            false,
            "Invalid path error handling"
        ));
    }
    catch (const std::runtime_error&)
    {
        record(check(
            true,
            "Invalid path error handling"
        ));
    }
    catch (const std::exception& e)
    {
        record(check(false, "Invalid path error handling"));
        std::cout << "Unexpected error: " << e.what() << '\n';
    }

    // TEST 6: Multiple Messages
    try
    {
        const std::string multiple_file = "multiple_messages.log";

        fs::remove(multiple_file);

        logging::FileSink sink(multiple_file);

        for (int i = 1; i <= 100; ++i)
        {
            sink.write("Log message " + std::to_string(i));
        }

        sink.flush();

        auto multiple_lines = readLines(multiple_file);

        record(check(
            multiple_lines.size() == 100 &&
            multiple_lines.front() == "Log message 1" &&
            multiple_lines.back() == "Log message 100",
            "Writing and verifying 100 messages"
        ));
    }
    catch (const std::exception& e)
    {
        record(check(false, "Writing 100 messages"));
        std::cout << "Multiple message error: " << e.what() << '\n';
    }

    std::cout << "\n===== Test Summary =====\n";
    std::cout << "Passed: " << passed << '\n';
    std::cout << "Failed: " << failed << '\n';

    if (failed == 0)
    {
        std::cout << "\nAll FileSink tests passed!\n";
    }
    else
    {
        std::cout << "\nSome FileSink tests failed.\n";
    }

    return failed == 0 ? 0 : 1;
}
