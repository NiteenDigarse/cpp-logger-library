
cmake_minimum_required(VERSION 3.16)

project(Logger VERSION 1.0.0 LANGUAGES CXX)

# ==========================================
# C++ Standard
# ==========================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==========================================
# Logger Static Library
# ==========================================

add_library(logger STATIC
    src/logger.cpp
    src/log_formatter.cpp
    src/console_sink.cpp
    src/file_sink.cpp
)

target_include_directories(logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ==========================================
# Example Executable
# ==========================================

add_executable(logger_example
    examples/logger_example.cpp
)

target_link_libraries(logger_example
    PRIVATE logger
)

# ==========================================
# Test Executables
# ==========================================

add_executable(test_log_level
    tests/test_log_level.cpp
)

target_link_libraries(test_log_level
    PRIVATE logger
)


add_executable(test_log_formatter
    tests/test_log_formatter.cpp
)

target_link_libraries(test_log_formatter
    PRIVATE logger
)


add_executable(test_console_sink
    tests/test_console_sink.cpp
)

target_link_libraries(test_console_sink
    PRIVATE logger
)


add_executable(test_file_sink
    tests/test_file_sink.cpp
)

target_link_libraries(test_file_sink
    PRIVATE logger
)


add_executable(test_logger
    tests/test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE logger
)

# ==========================================
# Enable Testing
# ==========================================

enable_testing()

# ==========================================
# Register Tests
# ==========================================

add_test(
    NAME LogLevelTest
    COMMAND test_log_level
)

add_test(
    NAME LogFormatterTest
    COMMAND test_log_formatter
)

add_test(
    NAME ConsoleSinkTest
    COMMAND test_console_sink
)

add_test(
    NAME FileSinkTest
    COMMAND test_file_sink
)

add_test(
    NAME LoggerTest
    COMMAND test_logger
)

find_package(Threads REQUIRED)

add_executable(logger_benchmark
    benchmarks/logger_benchmark.cpp
)

target_link_libraries(logger_benchmark
    PRIVATE
        logger
        Threads::Threads
)
