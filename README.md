![Logger CI](https://github.com/NiteenDigarse/cpp-logger-library/actions/workflows/ci.yml/badge.svg)

# C++ Logger Library

A lightweight, synchronous, thread-safe logging library built from scratch using **C++17**.

The project demonstrates practical C++ engineering concepts, including object-oriented design, dependency injection, RAII, mutex-based synchronization, exception handling, CMake, automated testing, and performance benchmarking.

## Features

- Six logging levels: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`
- Minimum log-level filtering
- Console and file logging destinations
- Multiple logging sinks
- Timestamp and thread ID in log records
- Custom log formatting
- Thread-safe logging through `std::mutex`
- Runtime log-level configuration
- Dependency injection for logging sinks
- File append mode
- Exception propagation from logging sinks
- Explicit validation of invalid log levels
- CMake-based build system
- Automated tests using CTest
- Multithreaded performance benchmark

## Architecture

The library follows a modular architecture in which logging responsibilities are separated into independent components.

```text
              Application
                   |
                   v
                Logger
                   |
            Log Level Filter
                   |
                   v
                LogRecord
                   |
                   v
              LogFormatter
                   |
          +--------+--------+
          |                 |
          v                 v
      ConsoleSink        FileSink
          |                 |
          v                 v
       Terminal          Log File
```

### Core Components

| Component | Responsibility |
|---|---|
| `Logger` | Public logging API, filtering, synchronization and sink coordination |
| `LogLevel` | Defines six logging severity levels |
| `LogRecord` | Stores message, severity, timestamp and thread ID |
| `LogFormatter` | Converts log records into formatted strings |
| `LogSink` | Abstract interface for logging destinations |
| `ConsoleSink` | Writes log messages to the console |
| `FileSink` | Writes log messages to a file |

## Project Structure

```text
09-logger/
├── .github/
│   └── workflows/
│       └── ci.yml
├── benchmarks/
│   ├── logger_benchmark.cpp
│   └── benchmark-results.txt
├── docs/
│   ├── 01-logger-project-overview.md
│   ├── ...
│   └── 15-performance-benchmarking-and-analysis.md
├── examples/
│   └── logger_example.cpp
├── include/
│   └── logger/
│       ├── logger.hpp
│       ├── log_level.hpp
│       ├── log_record.hpp
│       ├── log_formatter.hpp
│       ├── log_sink.hpp
│       ├── console_sink.hpp
│       └── file_sink.hpp
├── src/
│   ├── logger.cpp
│   ├── log_formatter.cpp
│   ├── console_sink.cpp
│   └── file_sink.cpp
├── tests/
│   ├── test_log_level.cpp
│   ├── test_log_formatter.cpp
│   ├── test_console_sink.cpp
│   ├── test_file_sink.cpp
│   └── test_logger.cpp
├── .gitignore
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Requirements

- C++17 compatible compiler
- CMake 3.16 or later
- Git

Tested development environment:

- GCC 16.2.0
- MinGW / MSYS2 UCRT64
- Windows
- CMake with MinGW Makefiles

## Build Instructions

### 1. Clone the repository

```bash
git clone <YOUR_GITHUB_REPOSITORY_URL>
cd 09-logger
```

### 2. Configure Debug Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

### 3. Build

```bash
cmake --build build --parallel
```

### 4. Run Tests

```bash
ctest --test-dir build --output-on-failure
```

### 5. Configure Release Build

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
```

### 6. Build Release

```bash
cmake --build build-release --parallel
```

### 7. Run Release Tests

```bash
ctest --test-dir build-release --output-on-failure
```

### Windows MinGW Build

For the MinGW Makefiles generator, use:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage Example

```cpp
#include "logger/logger.hpp"
#include "logger/console_sink.hpp"
#include "logger/file_sink.hpp"

#include <memory>
#include <vector>

int main() {
    using namespace logging;

    std::vector<std::shared_ptr<LogSink>> sinks;

    sinks.push_back(std::make_shared<ConsoleSink>());
    sinks.push_back(std::make_shared<FileSink>("application.log"));

    Logger logger(std::move(sinks), LogLevel::TRACE);

    logger.trace("Trace message");
    logger.debug("Debug message");
    logger.info("Application started");
    logger.warn("Low disk space");
    logger.error("Failed to process request");
    logger.fatal("Critical application failure");

    logger.flush();

    return 0;
}
```

## Example Log Output

```text
[2026-11-01 10:30:00] [INFO] [Thread: 1] Application started
[2026-11-01 10:30:01] [WARN] [Thread: 1] Low disk space
[2026-11-01 10:30:02] [ERROR] [Thread: 1] Failed to process request
```

*The timestamps and thread IDs above are illustrative examples.*

## Testing

The project includes five CTest test executables:

| Test Suite | Coverage |
|---|---|
| `LogLevelTest` | Log-level definitions and behavior |
| `LogFormatterTest` | Log formatting and timestamps |
| `ConsoleSinkTest` | Console output behavior |
| `FileSinkTest` | File output behavior |
| `LoggerTest` | Logger API, filtering, validation, exceptions and concurrency |

### Verified Test Results

| Configuration | CTest Targets | Logger Internal Tests |
|---|---:|---:|
| Debug | 5/5 passed | 18/18 passed |
| Release | 5/5 passed | 18/18 passed |

All reported tests passed in both Debug and Release configurations.

## Performance Benchmark

The benchmark uses a custom `CountingSink`, an atomic counter, warm-up iterations and five repetitions per scenario.

**Measured results:**

| Scenario | Operations | Median Throughput |
|---|---:|---:|
| Single-thread enabled | 100,000 | 77,233.36 ops/sec |
| Two-thread enabled | 200,000 | 67,498.79 ops/sec |
| Four-thread enabled | 400,000 | 67,050.41 ops/sec |
| Eight-thread enabled | 800,000 | 63,423.30 ops/sec |
| Four-thread filtered | 400,000 | 15,254,541.09 calls/sec |

**Benchmark environment:** GNU 16.2.0, MinGW/MSYS2 UCRT64, Windows, Release build.

All five benchmark scenarios reported successful message verification.

The enabled scenarios measure Logger overhead together with the `CountingSink` atomic counter. The filtered scenario measures attempted calls that are rejected by the configured log level.

These are measurements from this particular test environment, not universal performance guarantees. They do not represent real console or file I/O performance.

Detailed results are available in `benchmarks/benchmark-results.txt`.

## Thread Safety

The Logger uses:

- `std::mutex` for synchronization.
- `std::lock_guard` for RAII-based lock management.

Calls made through the same Logger instance are serialized while its mutex is held.

This protects Logger-managed state and prevents concurrent Logger calls from interleaving their sink operations.

## Design Decisions and Limitations

This project intentionally uses a synchronous logging architecture.

Current design decisions:

- Synchronous logging; no background worker.
- Console and file sinks.
- Dependency injection instead of a singleton.
- `std::shared_ptr` for sink ownership.
- Mutex-based synchronization.
- Sink exceptions propagate to the caller.
- If a sink throws, subsequent sinks in that operation are not called.

Known limitations:

- A slow sink can block other threads using the same Logger.
- A sink that re-enters the same Logger can cause deadlock because the Logger uses a non-recursive mutex.
- Direct concurrent use of a sink outside the Logger is not protected by the Logger's mutex.
- Stream flushing does not guarantee physical disk durability.
- Asynchronous logging, log rotation and per-sink exception isolation are not implemented.

These limitations are documented design trade-offs rather than claims of unsupported functionality.

## Concepts Demonstrated

- Modern C++17
- Object-Oriented Programming
- Abstract Classes and Virtual Functions
- Dependency Injection
- RAII
- Smart Pointers
- Move Semantics
- Exception Safety
- Mutex and Multithreading
- File I/O
- CMake
- CTest
- Automated Testing
- Performance Benchmarking
- GitHub Actions CI

## Future Improvements

Potential extensions for future versions:

- Asynchronous logging with a background worker
- Log rotation
- Configurable log formats
- Per-sink exception isolation
- Additional output destinations
- Performance and latency profiling

## Project Goal

The goal of this project is to understand how a practical logging system can be designed and implemented using modern C++ engineering principles.

Rather than relying on an existing logging library, this project focuses on implementing the core components, understanding their internal behavior, testing correctness, and measuring performance.

---

**Project:** C++ Engineering Projects — Project #09  
**Language:** C++17  
**Build System:** CMake  
**Testing:** CTest
