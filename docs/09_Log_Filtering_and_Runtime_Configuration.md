# 09. Log Filtering and Runtime Configuration

## 1. WHY — Why Do We Need Log Filtering?

Applications generate different types of log messages during execution.

For example:

```text
TRACE  - Detailed execution information
DEBUG  - Debugging information
INFO   - General application events
WARN   - Potential problems
ERROR  - Application errors
FATAL  - Critical failures
```

Not every application environment needs every type of message.

Consider two situations:

**Development Environment**

Developers may need detailed execution information.

```text
TRACE
DEBUG
INFO
WARN
ERROR
FATAL
```

**Production Environment**

A production application may only need warnings and serious errors.

```text
WARN
ERROR
FATAL
```

If the Logger writes every message regardless of its importance, the log file can become unnecessarily large and difficult to analyze.

To solve this problem, we implement log-level filtering.

## 2. Concept — What Is Log-Level Filtering?

Log-level filtering is a mechanism that decides whether a log message should be processed based on its severity.

Our Logger maintains a minimum accepted severity level.

The incoming message is compared against this configured threshold.

The rule is:

```cpp
if (level < min_level_) {
    return;
}
```

If the incoming level is less severe than the configured minimum level, the message is rejected.

Otherwise, the message is accepted and continues through the logging pipeline.

## 3. Understanding Severity Ordering

File: `include/logger/log_level.hpp`

```cpp
#pragma once

#include <cstdint>

namespace logging {

enum class LogLevel : std::uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
};

}
```

Each severity level has an explicit numeric value.

The numeric values establish the severity ordering.

| Log Level | Numeric Value | Severity |
|---|---:|---|
| TRACE | 0 | Lowest |
| DEBUG | 1 | Low |
| INFO | 2 | Normal |
| WARN | 3 | Elevated |
| ERROR | 4 | High |
| FATAL | 5 | Highest |

This ordering makes severity comparisons possible.

For example:

```cpp
LogLevel::INFO < LogLevel::ERROR
```

The comparison evaluates to true because INFO has a lower underlying value than ERROR.

**Important:** The numeric ordering is an intentional design decision. The filtering logic depends on it.

## 4. Mental Model — Minimum Severity Threshold

Think of the configured minimum level as a filter placed at the entrance of the logging pipeline.

```text
          Incoming Message
                 |
                 v
          Compare Severity
                 |
          +------+------+
          |             |
       Below         Meets or
       Minimum       Exceeds
          |             |
          v             v
       Reject        Accept
                        |
                        v
                    LogRecord
                        |
                        v
                   LogFormatter
                        |
                        v
                      Sinks
```

The Logger rejects filtered messages before creating a LogRecord or formatting the message.

This avoids unnecessary processing.

## 5. Understanding the Filtering Implementation

File: `src/logger.cpp`

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

### 5.1 Acquiring the Mutex

```cpp
std::lock_guard<std::mutex> lock(mutex_);
```

The Logger acquires its mutex before accessing the configured minimum level.

This ensures that concurrent logging and configuration operations through this Logger instance are synchronized.

### 5.2 Comparing Severity

```cpp
if (level < min_level_) {
    return;
}
```

This is the central filtering condition.

Suppose:

```cpp
min_level_ = LogLevel::WARN;
```

Now consider these comparisons:

```cpp
LogLevel::TRACE < LogLevel::WARN  // true
LogLevel::DEBUG < LogLevel::WARN  // true
LogLevel::INFO  < LogLevel::WARN  // true
LogLevel::WARN  < LogLevel::WARN  // false
LogLevel::ERROR < LogLevel::WARN  // false
LogLevel::FATAL < LogLevel::WARN  // false
```

Therefore, TRACE, DEBUG, and INFO are rejected.

WARN, ERROR, and FATAL are accepted.

### 5.3 Early Return

```cpp
return;
```

When the condition is true, the function exits immediately.

The following operations are skipped:

- LogRecord construction.
- Timestamp acquisition.
- Thread ID acquisition.
- Message formatting.
- Sink distribution.

This makes early filtering more efficient than formatting messages that will eventually be discarded.

## 6. Understanding Minimum Log Level

The Logger stores its configured threshold in:

```cpp
LogLevel min_level_;
```

The constructor accepts an optional minimum level:

```cpp
explicit Logger(
    std::vector<std::shared_ptr<LogSink>> sinks,
    LogLevel min_level = LogLevel::TRACE
);
```

If the application does not specify a minimum level, TRACE is used.

This means all six defined severity levels are accepted by default.

Example:

```cpp
logging::Logger logger(
    std::move(sinks),
    logging::LogLevel::INFO
);
```

This Logger accepts INFO, WARN, ERROR, and FATAL messages.

TRACE and DEBUG messages are filtered out.

## 7. Runtime Configuration

A useful Logger should allow its filtering behavior to change while the application is running.

For example:

- During development, enable DEBUG messages.
- During normal execution, use INFO.
- During troubleshooting, temporarily enable TRACE.
- In production, use WARN or ERROR.

Our Logger supports this through `setLevel()` and `getLevel()`.

### 7.1 setLevel()

Implementation:

```cpp
void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}
```

This function updates the minimum accepted severity.

The mutex protects the shared configuration variable.

Example:

```cpp
logger.setLevel(LogLevel::ERROR);
```

After this call, INFO and WARN messages are filtered out, while ERROR and FATAL messages are accepted.

### 7.2 getLevel()

Implementation:

```cpp
LogLevel Logger::getLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return min_level_;
}
```

This function returns the currently configured minimum severity.

Example:

```cpp
LogLevel current_level = logger.getLevel();
```

The method is const because reading the configuration does not logically modify the Logger.

The mutex is declared mutable so it can still be locked inside this const member function.

## 8. Runtime Configuration Example

```cpp
#include "logger/logger.hpp"

int main() {

    // Assume logger has already been constructed.

    logger.setLevel(logging::LogLevel::ERROR);

    logger.info("Application started");
    logger.warn("Low memory warning");
    logger.error("Database connection failed");

    logger.setLevel(logging::LogLevel::DEBUG);

    logger.debug("Debugging information");
    logger.info("Request processed");

    logger.flush();

    return 0;
}
```

Expected accepted messages:

```text
[ERROR] Database connection failed
[DEBUG] Debugging information
[INFO] Request processed
```

The INFO and WARN messages generated before the configuration change are rejected.

After changing the threshold to DEBUG, DEBUG, INFO, WARN, ERROR, and FATAL messages are accepted.

## 9. Practical Experiment — Verify Filtering

Use the existing Logger example or create a small experiment.

Configure the Logger with:

```cpp
logging::Logger logger(
    std::move(sinks),
    logging::LogLevel::WARN
);
```

Generate messages:

```cpp
logger.trace("Trace message");
logger.debug("Debug message");
logger.info("Info message");
logger.warn("Warning message");
logger.error("Error message");
logger.fatal("Fatal message");
```

Expected output:

```text
[WARN] Warning message
[ERROR] Error message
[FATAL] Fatal message
```

The first three messages should not appear.

## 10. Practical Experiment — Change Level at Runtime

After the first experiment, change the minimum level:

```cpp
logger.setLevel(logging::LogLevel::DEBUG);

logger.debug("Debug message after configuration change");
logger.info("Info message after configuration change");
```

Expected output should now include:

```text
[DEBUG] Debug message after configuration change
[INFO] Info message after configuration change
```

This confirms that filtering behavior can change without reconstructing the Logger object.

## 11. Unit Testing

File: `tests/test_logger.cpp`

Our Logger tests cover filtering and runtime configuration.

### Test 1 — Minimum Level Filtering

The test configures the Logger with WARN as its minimum level.

It sends messages at different severity levels.

Expected accepted messages:

```text
WARN
ERROR
FATAL
```

Expected rejected messages:

```text
TRACE
DEBUG
INFO
```

### Test 2 — Runtime Level Change

The test initially configures ERROR as the minimum level.

It verifies that INFO is rejected.

Then it changes the threshold to INFO using:

```cpp
logger.setLevel(LogLevel::INFO);
```

It verifies that a subsequent INFO message is accepted.

### Test 3 — Reading the Current Level

The test uses:

```cpp
logger.getLevel();
```

It checks the initial configuration and verifies the updated level after calling setLevel().

Run the complete test suite:

```bash
ctest --test-dir build --output-on-failure
```

The filtering and runtime configuration behaviors are covered by the LoggerTest suite.

## 12. Thread Safety of Runtime Configuration

Our implementation uses the same mutex in:

- `log()`
- `setLevel()`
- `getLevel()`
- `flush()`

This protects access to the Logger's shared state.

For example, if one thread changes the minimum severity while another thread logs a message, both operations use the same mutex.

The operations are therefore serialized through this Logger instance.

The exact threshold observed by a logging operation depends on which operation acquires the mutex first.

## 13. Current Design Limitations

The current filtering implementation has some limitations.

### 13.1 Invalid Enum Values

Although LogLevel defines six valid values, C++ allows an enum value to be created using an explicit cast.

Example:

```cpp
auto invalid_level = static_cast<LogLevel>(255);
```

Our current `setLevel()` implementation does not validate such values.

The current comparison-based filtering logic assumes valid severity values.

Input validation can be added as an engineering improvement.

### 13.2 No Per-Sink Filtering

Currently, the minimum level is configured for the entire Logger.

All configured sinks receive the same accepted messages.

For example, we cannot currently configure:

- ConsoleSink to receive DEBUG and above.
- FileSink to receive ERROR and above.

Per-sink filtering would require additional design.

### 13.3 No Configuration File

The minimum severity is currently configured through the C++ API.

The Logger does not yet load logging configuration from an external file.

## 14. Important Engineering Points

- Log levels have an intentional numeric severity ordering.
- The minimum level acts as a filtering threshold.
- Lower-severity messages are rejected.
- Equal or higher-severity messages are accepted.
- Filtering happens before LogRecord construction.
- Early return avoids unnecessary formatting and sink operations.
- `setLevel()` changes the threshold at runtime.
- `getLevel()` reads the current threshold.
- A mutex synchronizes configuration and logging operations.
- The current implementation uses one global threshold per Logger.
- Invalid enum values are not currently validated.

## 15. Quick Reference

| Feature | Implementation |
|---|---|
| Severity ordering | `enum class LogLevel` |
| Minimum threshold | `LogLevel min_level_` |
| Filtering | `if (level < min_level_)` |
| Runtime update | `setLevel()` |
| Configuration read | `getLevel()` |
| Synchronization | `std::mutex` |
| Automatic lock management | `std::lock_guard` |

## 16. Progress

**Completed:**

- Severity ordering.
- Minimum log-level configuration.
- Comparison-based filtering.
- Early return for rejected messages.
- Runtime level updates.
- Current level retrieval.
- Mutex-protected configuration.
- Filtering unit tests.
- Runtime configuration unit tests.

**Next Topic:**

`10-mutex-lock-guard-and-thread-safety.md`

We will study mutexes, critical sections, lock_guard, concurrent access, and how synchronization works internally in our Logger implementation.