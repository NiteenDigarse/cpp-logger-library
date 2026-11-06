# 13. Test Doubles and Concurrent Testing

## 1. WHY — Why Do We Need Test Doubles?

When testing a component, it may depend on other components or external resources.

For example, our Logger depends on LogSink implementations.

The production sinks are:

- ConsoleSink
- FileSink

If every Logger test uses these real implementations, testing can become more complicated.

For example:

- Console output must be captured.
- Temporary log files must be created and inspected.
- File-system permissions may affect test results.
- External output behavior can make tests harder to isolate.

We need a way to test Logger without depending on real output destinations.

This is where **Test Doubles** become useful.

## 2. Concept — What Are Test Doubles?

A test double is an object that replaces a real dependency during testing.

It provides the behavior required by the test without necessarily performing the real dependency's complete work.

Common categories include:

| Type | Purpose |
|---|---|
| Dummy | Passed into code but not meaningfully used |
| Stub | Returns predefined responses |
| Fake | Provides a simplified working implementation |
| Mock | Verifies expected interactions |
| Spy | Records information about how it was used |

These are general testing terms. Our current Logger project uses a custom recording sink that behaves primarily like a **Fake** and also records interactions for verification.

## 3. Mental Model — Replacing a Real Dependency

Production architecture:

```text
Application
    |
    v
  Logger
    |
    v
 ConsoleSink
    |
    v
  Terminal
```

Testing architecture:

```text
Test Program
     |
     v
   Logger
     |
     v
   TestSink
     |
     v
In-Memory Message Storage
```

The Logger does not need to know whether it is receiving a real ConsoleSink, FileSink, or TestSink.

It only requires an implementation of the LogSink interface.

This is possible because our architecture uses interface-based polymorphism and dependency injection.

## 4. The LogSink Interface

File: `include/logger/log_sink.hpp`

```cpp
#pragma once

#include <string>

namespace logging {

class LogSink {
public:
    virtual ~LogSink() = default;

    virtual void write(const std::string& message) = 0;

    virtual void flush() = 0;
};

}
```

The interface defines two required operations:

- `write()` receives a formatted log message.
- `flush()` requests that buffered output be delivered.

Any concrete sink must implement these operations.

TestSink can therefore inherit from LogSink just like ConsoleSink and FileSink.

## 5. Implementing a Custom TestSink

File: `tests/test_logger.cpp`

Our custom TestSink stores received messages in memory instead of writing them to a terminal or file.

```cpp
class TestSink : public logging::LogSink {

private:
    mutable std::mutex mutex_;

    std::vector<std::string> messages_;

    int flush_count_ = 0;

public:

    void write(const std::string& message) override {

        std::lock_guard<std::mutex> lock(mutex_);

        messages_.push_back(message);
    }

    void flush() override {

        std::lock_guard<std::mutex> lock(mutex_);

        ++flush_count_;
    }

    std::vector<std::string> getMessages() const {

        std::lock_guard<std::mutex> lock(mutex_);

        return messages_;
    }

    std::size_t messageCount() const {

        std::lock_guard<std::mutex> lock(mutex_);

        return messages_.size();
    }

    int flushCount() const {

        std::lock_guard<std::mutex> lock(mutex_);

        return flush_count_;
    }
};
```

### 5.1 Internal Message Storage

```cpp
std::vector<std::string> messages_;
```

This vector stores every message received by TestSink.

Unlike FileSink, it does not write anything to the file system.

### 5.2 Recording Messages

```cpp
void write(const std::string& message) override {

    std::lock_guard<std::mutex> lock(mutex_);

    messages_.push_back(message);
}
```

Whenever Logger calls `write()`, TestSink stores the supplied message.

The mutex protects the internal vector against concurrent access.

### 5.3 Recording Flush Calls

```cpp
int flush_count_ = 0;
```

Instead of flushing an actual output destination, TestSink increments a counter.

```cpp
void flush() override {

    std::lock_guard<std::mutex> lock(mutex_);

    ++flush_count_;
}
```

This allows tests to verify whether Logger requested a flush.

### 5.4 Reading Stored Messages

```cpp
std::vector<std::string> getMessages() const {

    std::lock_guard<std::mutex> lock(mutex_);

    return messages_;
}
```

The method returns a copy of the stored messages.

Returning a copy prevents callers from directly modifying the internal vector.

The mutex protects the copying operation.

## 6. Injecting TestSink into Logger

Our Logger receives its dependencies through its constructor.

Therefore, TestSink can be injected without changing Logger's production implementation.

Example:

```cpp
auto test_sink = std::make_shared<TestSink>();

std::vector<std::shared_ptr<logging::LogSink>> sinks;

sinks.push_back(test_sink);

logging::Logger logger(std::move(sinks));
```

Now Logger sends messages to TestSink.

The test can inspect those messages using:

```cpp
test_sink->getMessages();
```

This is an example of testing through an abstraction rather than depending on a concrete output destination.

## 7. Testing Log-Level Filtering with TestSink

Suppose we want to verify that Logger rejects messages below WARN.

We configure:

```cpp
auto test_sink = std::make_shared<TestSink>();

std::vector<std::shared_ptr<logging::LogSink>> sinks;

sinks.push_back(test_sink);

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

Now inspect the stored messages:

```cpp
auto messages = test_sink->getMessages();
```

Expected message count:

```text
3
```

The accepted levels are:

```text
WARN
ERROR
FATAL
```

The rejected levels are:

```text
TRACE
DEBUG
INFO
```

This verifies filtering without requiring terminal or file output.

## 8. Testing Multiple Sinks

TestSink can also help verify that Logger distributes messages to multiple destinations.

Example:

```cpp
auto sink1 = std::make_shared<TestSink>();
auto sink2 = std::make_shared<TestSink>();

std::vector<std::shared_ptr<logging::LogSink>> sinks;

sinks.push_back(sink1);
sinks.push_back(sink2);

logging::Logger logger(std::move(sinks));

logger.info("Application started");
```

Expected behavior:

```text
sink1 receives: Application started
sink2 receives: Application started
```

Both sinks should receive the same formatted message.

This verifies the Logger's sink-distribution behavior.

## 9. Concurrent Testing — WHY?

Our Logger is designed to support calls from multiple threads.

A single-threaded test cannot verify behavior under concurrent access.

We therefore need tests that create multiple threads and allow them to log through the same Logger instance.

The objective is to verify that messages are not lost and that the stored results remain valid.

## 10. Creating Multiple Threads

Our Logger test uses four worker threads.

Each thread generates 100 messages.

```cpp
std::vector<std::thread> threads;

for (int i = 0; i < 4; ++i) {

    threads.emplace_back([&logger]() {

        for (int j = 0; j < 100; ++j) {

            logger.info("Concurrent message");
        }
    });
}
```

### Understanding the Loop

The outer loop creates four threads.

The inner loop executes 100 logging operations in each thread.

Total expected messages:

\[
4 \times 100 = 400
\]

## 11. Understanding std::thread

```cpp
threads.emplace_back([&logger]() {
    // Thread work
});
```

This creates a new thread that executes the supplied callable.

The lambda captures `logger` by reference.

All worker threads therefore use the same Logger instance.

This is important because we want to test concurrent access to one shared Logger rather than separate Logger objects.

## 12. Understanding join()

After creating the worker threads, the main thread waits for them to finish.

```cpp
for (auto& thread : threads) {

    thread.join();
}
```

`join()` blocks the calling thread until the associated worker thread completes.

Without joining the worker threads, the test could inspect the message count before all logging operations have finished.

Joining also ensures that the test does not destroy the Logger or TestSink while worker threads are still using them.

## 13. Verifying the Final Message Count

After all threads have completed:

```cpp
std::size_t count = test_sink->messageCount();
```

Expected result:

```text
Expected messages: 400
Actual messages:   400
```

The test passes if the actual message count matches the expected count.

This verifies that all generated logging operations were recorded in this test execution.

## 14. Testing Message Integrity

Counting messages alone is not sufficient.

We should also verify that received messages contain the expected information.

For example, the test can inspect every stored message:

```cpp
auto messages = test_sink->getMessages();

bool valid = true;

for (const auto& message : messages) {

    if (message.find("[INFO]") == std::string::npos ||
        message.find("Thread ") == std::string::npos) {

        valid = false;
        break;
    }
}
```

The test verifies:

- Every received message contains the INFO severity.
- Every received message contains thread information.

This checks basic message integrity.

It does not prove that every thread produced exactly 100 messages or that all thread IDs are unique.

Those would require additional test assertions.

## 15. Why Does TestSink Need Its Own Mutex?

Our Logger already uses a mutex.

Why does TestSink need another mutex?

Because TestSink owns its own internal state:

```cpp
std::vector<std::string> messages_;
int flush_count_ = 0;
```

The TestSink must protect that state whenever it is accessed.

Its mutex protects:

- Message insertion.
- Message retrieval.
- Message count retrieval.
- Flush count updates.

This is separate from the Logger's synchronization responsibility.

The Logger mutex protects operations through Logger.

The TestSink mutex protects TestSink's internal data.

Each component is responsible for its own shared mutable state.

## 16. Practical Testing Commands

Build the project:

```bash
cmake --build build
```

Run the Logger test directly:

```bash
.\build\test_logger.exe
```

Run the complete test suite:

```bash
ctest --test-dir build --output-on-failure
```

The current project has five registered test suites.

The Logger test suite contains the custom TestSink and multithreaded scenarios.

## 17. Important Testing Limitations

Our current TestSink and concurrent tests provide useful evidence, but they are not exhaustive.

Possible future improvements:

- Add a configurable sink that deliberately throws exceptions.
- Verify how Logger behaves when one sink fails.
- Verify that every worker thread contributes exactly 100 messages.
- Verify that thread identifiers are represented correctly.
- Test concurrent `setLevel()` and `log()` calls.
- Run ThreadSanitizer where the compiler and environment support it.
- Add tests for reentrant logging and deadlock-sensitive scenarios.

A passing concurrent test does not prove the absence of every possible race condition.

## 18. Important Engineering Points

- A test double replaces a real dependency during testing.
- TestSink implements the LogSink interface.
- TestSink records messages in memory.
- Dependency injection makes Logger independently testable.
- A recording fake can verify received messages.
- TestSink uses its own mutex to protect internal state.
- Multiple threads can share one Logger instance.
- `std::thread` creates worker threads.
- `join()` waits for worker completion.
- Expected message count can verify message delivery.
- Message-content checks provide additional integrity validation.
- Passing multithreaded tests do not prove complete thread safety.

## 19. Quick Reference

| Concept | Purpose |
|---|---|
| Test Double | Replaces a real dependency during testing |
| Fake | Simplified working implementation |
| Mock | Verifies expected interactions |
| Spy | Records interactions for later inspection |
| TestSink | Custom in-memory LogSink |
| Dependency Injection | Supplies TestSink to Logger |
| `std::thread` | Creates worker threads |
| `join()` | Waits for thread completion |
| `getMessages()` | Retrieves recorded messages |
| `messageCount()` | Returns stored message count |
| `std::mutex` | Protects TestSink's internal state |

## 20. Progress

**Completed:**

- Custom TestSink implementation.
- LogSink interface reuse.
- In-memory message storage.
- Flush-call recording.
- TestSink mutex protection.
- Dependency injection in Logger tests.
- Multiple-sink testing.
- Four-thread concurrent logging test.
- 400-message count verification.
- Basic message-integrity verification.

**Next Topic:**

`14-cmake-build-system-and-ctest.md`

We will study how CMake builds our Logger static library, how executables link against it, how test executables are registered, and how CTest executes the complete test suite.