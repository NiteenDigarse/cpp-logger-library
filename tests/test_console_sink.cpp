
#include "logger/console_sink.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

int main()
{
    try
    {
        logging::ConsoleSink sink;

        std::ostringstream captured_output;

        // Capture std::cout output
        std::streambuf* original_buffer =
            std::cout.rdbuf(captured_output.rdbuf());

        try
        {
            // TEST 1: Basic message
            sink.write("Application started");

            // TEST 2: Multiple messages
            sink.write("Database connected");
            sink.write("Logger initialized");

            // TEST 3: Empty message
            sink.write("");

            // TEST 4: Special characters
            sink.write("Error: File not found! @#$%");

            // TEST 5: Flush
            sink.flush();
        }
        catch (...)
        {
            std::cout.rdbuf(original_buffer);
            throw;
        }

        // Restore original console output
        std::cout.rdbuf(original_buffer);

        const std::string output = captured_output.str();

        // Verify basic message
        expect(
            output.find("Application started\n") != std::string::npos,
            "Basic message writing failed"
        );

        // Verify multiple messages
        expect(
            output.find("Database connected\n") != std::string::npos &&
            output.find("Logger initialized\n") != std::string::npos,
            "Multiple message writing failed"
        );

        // Verify empty message
        expect(
            output.find("\n\n") != std::string::npos,
            "Empty message handling failed"
        );

        // Verify special characters
        expect(
            output.find("Error: File not found! @#$%\n") != std::string::npos,
            "Special character handling failed"
        );

        // Verify total number of lines
        std::size_t newline_count = 0;

        for (char ch : output)
        {
            if (ch == '\n')
            {
                ++newline_count;
            }
        }

        expect(
            newline_count == 5,
            "Unexpected number of output lines"
        );

        std::cout << "[PASS] Basic message writing\n";
        std::cout << "[PASS] Multiple message writing\n";
        std::cout << "[PASS] Empty message handling\n";
        std::cout << "[PASS] Special character handling\n";
        std::cout << "[PASS] Output line count\n";
        std::cout << "[PASS] Flush execution\n";

        std::cout << "\nAll ConsoleSink tests passed!\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nTest failed: " << e.what() << '\n';

        return 1;
    }
}
