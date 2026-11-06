# 08. Logger Class Design and Dependency Injection

## 1. WHY — Why Do We Need a Logger Class?

Until now, we have implemented several independent components:

- LogLevel
- LogRecord
- LogFormatter
- LogSink
- ConsoleSink
- FileSink

Each component has a specific responsibility.

However, an application needs a single interface through which it can generate and distribute log messages.

Without a central Logger, application code would need to manage formatting, output destinations, and log-level filtering independently.

This would create unnecessary coupling between application code and logging implementation details.

To solve this problem, we introduce the `Logger` class.

Its responsibilities are:

1. Accept logging requests from the application.
2. Filter messages according to the configured minimum log level.
3. Create structured LogRecord objects.
4. Format log records into strings.
5. Distribute formatted messages to configured sinks.
6. Synchronize access to shared Logger state.
7. Flush all configured sinks when requested.

## 2. Logger Architecture

The Logger acts as the central coordinator of the logging system.

```text
                 Application
                      |
                      v
                   Logger
                      |
              Minimum Log Level
                  Filtering
                      |
                      v
                  LogRecord
                      |
                      v
                 LogFormatter
                      |
              Formatted Message
                      |
             +--------+--------+
             |                 |
             v                 v
        ConsoleSink          FileSink
             |                 |
             v                 v
          Terminal          Log File
```

The important architectural decision is that Logger does not directly implement terminal or file output.

Instead, it communicates through the abstract `LogSink` interface.

This allows different output destinations to be added without changing the central logging logic.

## 3. Logger Class Declaration

File: `include/logger/logger.hpp`

```cpp
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "logger/log_level.hpp"
#include "logger/log_record.hpp"
#include "logger/log_sink.hpp"

namespace logging {

class Logger {
public:
    explicit Logger(
        std::vector<std::shared_ptr<LogSink>> sinks,
        LogLevel min_level = LogLevel::TRACE
    );

    void setLevel(LogLevel level);
    LogLevel getLevel() const;

    void log(LogLevel level, const std::string& message);

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
```

## 4. Understanding Logger Dependencies

The Logger header includes several standard library headers.

### 4.1 `<memory>`

```cpp
#include <memory>
```

This header provides smart pointers such as:

- `std::unique_ptr`
- `std::shared_ptr`
- `std::weak_ptr`

Our Logger uses `std::shared_ptr` to hold references to LogSink objects.

### 4.2 `<mutex>`

```cpp
#include <mutex>
```

This header provides synchronization mechanisms.

Our implementation uses `std::mutex` to protect shared Logger state and serialize logging operations.

### 4.3 `<vector>`

```cpp
#include <vector>
```

The Logger supports multiple output destinations.

A vector provides a convenient container for storing multiple sink pointers.

### 4.4 `<string>`

```cpp
#include <string>
```

The Logger accepts log messages as `std::string` objects.

## 5. Dependency Injection

### 5.1 What Is Dependency Injection?

Dependency Injection is a design technique in which an object receives the dependencies it needs from external code rather than creating those dependencies internally.

Our Logger depends on LogSink objects.

Instead of constructing ConsoleSink and FileSink internally, the Logger receives its sinks through its constructor.

Example:

```cpp
auto console =
    std::make_shared<logging::ConsoleSink>();

auto file =
    std::make_shared<logging::FileSink>("application.log");

std::vector<std::shared_ptr<logging::LogSink>> sinks;

sinks.push_back(console);
sinks.push_back(file);

logging::Logger logger(
    std::move(sinks),
    logging::LogLevel::INFO
);
```

The application creates the dependencies and passes them to Logger.

### 5.2 Why Is Dependency Injection Useful?

It provides several benefits:

- Logger does not depend directly on ConsoleSink.
- Logger does not depend directly on FileSink.
- Different sinks can be supplied without modifying Logger.
- Testing becomes easier because custom test sinks can be injected.
- The system becomes easier to extend.

This is an example of programming against an abstraction rather than a concrete implementation.

## 6. Understanding shared_ptr and Polymorphism

Our constructor accepts:

```cpp
std::vector<std::shared_ptr<LogSink>> sinks
```

Why can this vector hold both ConsoleSink and FileSink objects?

Because both classes publicly inherit from LogSink.

A `std::shared_ptr<ConsoleSink>` can be converted into a `std::shared_ptr<LogSink>`.

Similarly, a `std::shared_ptr<FileSink>` can be converted into a `std::shared_ptr<LogSink>`.

This allows the vector to store different concrete sink types through one common interface.

### Mental Model

```text
vector<shared_ptr<LogSink>>
              |
      +-------+-------+
      |               |
      v               v
 shared_ptr       shared_ptr
 ConsoleSink      FileSink
      |               |
      v               v
  Terminal         Log File
```

The Logger does not need to know the concrete type of each sink.

When it calls:

```cpp
sink->write(message);
```

virtual dispatch invokes the correct implementation.

### Why shared_ptr?

A shared pointer provides shared ownership of an object.

In our current design, the application can retain its own shared pointer while Logger also holds a shared pointer to the same sink.

The sink remains alive while at least one owning shared pointer remains.

**Important:** Shared ownership is a design choice here, not an absolute requirement. A different architecture could use unique ownership or non-owning references.

## 7. Constructor Implementation

File: `src/logger.cpp`

```cpp
Logger::Logger(
    std::vector<std::shared_ptr<LogSink>> sinks,
    LogLevel min_level
)
    : sinks_(std::move(sinks)),
      min_level_(min_level)
{
    if (sinks_.empty()) {
        throw std::invalid_argument(
            "Logger requires at least one sink"
        );
    }

    for (const auto& sink : sinks_) {
        if (!sink) {
            throw std::invalid_argument(
                "Logger cannot contain a null sink"
            );
        }
    }
}
```

### 7.1 Member Initializer List

```cpp
: sinks_(std::move(sinks)),
  min_level_(min_level)
```

This is a member initializer list.

It initializes class members before the constructor body executes.

### 7.2 Why std::move?

```cpp
sinks_(std::move(sinks))
```

The constructor receives its vector by value.

Using `std::move` allows the vector's resources to be transferred into the member vector instead of copying all its elements.

This is useful when the caller provides a temporary vector or explicitly moves an existing vector.

`std::move` itself does not move resources. It enables move semantics by converting its argument into an rvalue expression.

### 7.3 Constructor Validation

```cpp
if (sinks_.empty())
```

A Logger without any configured sinks cannot deliver messages anywhere.

Our implementation rejects this configuration.

Next:

```cpp
for (const auto& sink : sinks_)
```

We inspect every sink pointer.

```cpp
if (!sink)
```

A default-constructed or empty shared pointer evaluates to false.

The constructor rejects null sink pointers to prevent invalid dereferencing during logging.

## 8. Minimum Log Level Configuration

The Logger stores:

```cpp
LogLevel min_level_;
```

This variable controls which messages are accepted.

For example, when the minimum level is WARN:

| Log Level | Result |
|---|---|
| TRACE | Ignored |
| DEBUG | Ignored |
| INFO | Ignored |
| WARN | Accepted |
| ERROR | Accepted |
| FATAL | Accepted |

The severity values in our LogLevel enum are ordered from least severe to most severe.

Therefore, filtering can be implemented using an enum comparison.

## 9. Thread-Safe Level Configuration

### setLevel()

```cpp
void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}
```

This function changes the minimum accepted log level.

The mutex protects the shared `min_level_` variable.

### getLevel()

```cpp
LogLevel Logger::getLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return min_level_;
}
```

This function reads the current minimum log level.

It also acquires the same mutex to prevent an unsynchronized read while another thread modifies the value.

### Why Is mutex_ Mutable?

```cpp
mutable std::mutex mutex_;
```

The `getLevel()` method is const.

A const member function cannot normally modify non-mutable data members.

However, synchronization is an internal implementation detail. Locking a mutex does not logically change the Logger's observable configuration.

The `mutable` keyword allows the mutex to be locked inside this const member function.

## 10. Main Logging Operation

The most important function in Logger is `log()`.

```cpp
void Logger::log(
    LogLevel level,
    const std::string& message
) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (level < min_level_) {
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

    for (const auto& sink : sinks_) {
        sink->write(formatted_message);
    }
}
```

### 10.1 Lock Acquisition

```cpp
std::lock_guard<std::mutex> lock(mutex_);
```

The function acquires the Logger mutex.

Only one thread can hold this mutex at a time.

The lock is automatically released when the lock_guard object is destroyed.

This includes normal function exit and exception unwinding.

### 10.2 Log-Level Filtering

```cpp
if (level < min_level_) {
    return;
}
```

If the incoming message is less severe than the configured minimum level, the function returns immediately.

The message is not formatted or sent to any sink.

This avoids unnecessary work for filtered messages.

### 10.3 Creating LogRecord

```cpp
LogRecord record{
    message,
    level,
    std::chrono::system_clock::now(),
    std::this_thread::get_id()
};
```

A structured record captures:

- The original message.
- Its severity level.
- The current system timestamp.
- The ID of the current thread.

### 10.4 Formatting

```cpp
const std::string formatted_message =
    LogFormatter::format(record);
```

The Logger delegates formatting to LogFormatter.

This keeps message formatting separate from logging coordination.

### 10.5 Distributing to Sinks

```cpp
for (const auto& sink : sinks_) {
    sink->write(formatted_message);
}
```

The Logger iterates over every configured sink.

Each sink receives the same formatted message.

Virtual dispatch invokes the appropriate implementation, such as ConsoleSink or FileSink.

## 11. Convenience Logging Methods

The Logger provides convenient methods for each severity level.

```cpp
void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}
```

These functions delegate to the central `log()` implementation.

This avoids duplicating filtering, record creation, formatting, and sink distribution logic.

For example:

```cpp
logger.info("Application started");
```

Internally, this calls:

```cpp
logger.log(
    LogLevel::INFO,
    "Application started"
);
```

## 12. Flush Operation

```cpp
void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& sink : sinks_) {
        sink->flush();
    }
}
```

The Logger requests a flush from every configured sink.

For ConsoleSink, this flushes standard output.

For FileSink, this flushes the file stream.

The Logger does not need to know the internal flushing mechanism of each sink.

## 13. Practical Usage

File: `examples/logger_example.cpp`

```cpp
#include "logger/logger.hpp"
#include "logger/console_sink.hpp"
#include "logger/file_sink.hpp"

#include <memory>
#include <vector>
#include <utility>

int main() {

    auto console_sink =
        std::make_shared<logging::ConsoleSink>();

    auto file_sink =
        std::make_shared<logging::FileSink>("application.log");

    std::vector<std::shared_ptr<logging::LogSink>> sinks;

    sinks.push_back(console_sink);
    sinks.push_back(file_sink);

    logging::Logger logger(
        std::move(sinks),
        logging::LogLevel::TRACE
    );

    logger.trace("Trace message");
    logger.debug("Debug message");
    logger.info("Application started");
    logger.warn("Low memory warning");
    logger.error("Database connection failed");
    logger.fatal("Critical system failure");

    logger.flush();

    return 0;
}
```

Expected behavior:

- Every accepted message appears in the terminal.
- Every accepted message is written to application.log.
- Both destinations receive the same formatted log message.
- The Logger flushes both destinations at the end.

## 14. Current Design Limitations

The current Logger is a synchronous implementation.

This means logging work is performed by the calling thread.

The Logger also holds its mutex while calling external sink methods.

This gives us serialized access to the configured sinks through this Logger instance, but it has trade-offs:

- A slow sink blocks other logging threads.
- A sink that throws an exception interrupts the current logging operation.
- A custom sink that re-enters the same Logger can cause a deadlock.
- The mutex protects calls made through this Logger, not arbitrary direct calls to a sink from elsewhere.

These are important design considerations for the later engineering review.

Asynchronous logging and background worker threads are intentionally outside the current implementation.

## 15. Important Engineering Concepts

| Concept | Role in Logger |
|---|---|
| Dependency Injection | Supplies sinks externally |
| Abstract Interface | Allows different sink implementations |
| Polymorphism | Invokes the correct sink implementation |
| shared_ptr | Provides shared ownership |
| vector | Stores multiple sinks |
| std::move | Enables efficient transfer of vector resources |
| std::mutex | Protects shared Logger state |
| std::lock_guard | Provides automatic lock management |
| const | Prevents logical modification through getLevel() |
| mutable | Allows synchronization in const methods |
| RAII | Manages mutex locking lifetime |

## 16. Quick Reference

| Function | Responsibility |
|---|---|
| Logger() | Initializes and validates sinks |
| setLevel() | Changes the minimum severity |
| getLevel() | Returns the current severity threshold |
| log() | Central logging operation |
| trace() | Logs TRACE messages |
| debug() | Logs DEBUG messages |
| info() | Logs INFO messages |
| warn() | Logs WARN messages |
| error() | Logs ERROR messages |
| fatal() | Logs FATAL messages |
| flush() | Flushes all configured sinks |

## 17. Progress

**Completed:**

- Logger class declaration.
- Constructor and sink validation.
- Dependency injection.
- Shared ownership using shared_ptr.
- Minimum log-level configuration.
- Log filtering.
- LogRecord creation.
- Formatting integration.
- Multiple sink distribution.
- Thread synchronization.
- Convenience logging methods.
- Flush operation.
- Logger unit tests.

**Next Topic:**

`09-log-filtering-and-runtime-configuration.md`

We will study severity filtering, enum comparisons, runtime level changes, and how logging configuration affects application behavior.