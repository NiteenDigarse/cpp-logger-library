# 06. ConsoleSink Implementation

## 1. WHY — Why Do We Need ConsoleSink?

A logging system needs a mechanism to deliver log messages to an output destination.

During application development and debugging, developers frequently need to observe application behavior directly through the terminal.

For example:

```text
[2026-11-06 11:29:58] [INFO] [Thread: 1] Application started
[2026-11-06 11:29:58] [WARN] [Thread: 1] Low memory warning
[2026-11-06 11:29:58] [ERROR] [Thread: 1] Database connection failed
```

Instead of making the Logger responsible for directly handling terminal output, we create a separate component called `ConsoleSink`.

Its responsibility is simple:

- Receive a formatted log message.
- Write the message to standard output.
- Provide a flush operation.

This separation keeps the Logger independent of the actual output destination.

## 2. Concept — What Is a ConsoleSink?

`ConsoleSink` is a concrete implementation of the abstract `LogSink` interface.

It uses the C++ standard output stream:

```cpp
std::cout
```

The `std::cout` object represents the standard output stream, which normally displays information in the terminal.

Our design uses interface-based polymorphism.

The abstract interface defines what a sink must do, while `ConsoleSink` defines how terminal output is performed.

## 3. Mental Model

Consider the following flow:

```text
Logger
  |
  | Formatted message
  v
ConsoleSink
  |
  | std::cout
  v
Terminal
```

The Logger does not need to know whether the message is being written to a terminal or a file.

It only knows that every sink supports the `write()` and `flush()` operations.

## 4. Implementation

### Header File

File: `include/logger/console_sink.hpp`

```cpp
#pragma once

#include <string>
#include "logger/log_sink.hpp"

namespace logging {

class ConsoleSink : public LogSink {
public:
    void write(const std::string& message) override;
    void flush() override;
};

}
```

### Explanation

**Inheritance**

```cpp
class ConsoleSink : public LogSink
```

ConsoleSink publicly inherits from LogSink.

This allows ConsoleSink to be used wherever a LogSink object is expected.

**Override**

```cpp
void write(const std::string& message) override;
```

The `override` keyword tells the compiler that this function overrides a virtual function declared in the base class.

It also helps detect accidental signature mismatches.

## 5. Source Implementation

File: `src/console_sink.cpp`

```cpp
#include "logger/console_sink.hpp"
#include <iostream>

namespace logging {

void ConsoleSink::write(const std::string& message) {
    std::cout << message << '\\n';
}

void ConsoleSink::flush() {
    std::cout.flush();
}

}
```

### Understanding write()

```cpp
std::cout << message << '\\n';
```

This statement inserts the supplied message into the standard output stream.

The newline character moves the output position to the next line.

It does not explicitly request a flush in the same way that `std::endl` does.

### Understanding flush()

```cpp
std::cout.flush();
```

Output streams may buffer data before sending it to their destination.

The `flush()` operation requests that buffered output be delivered to the underlying destination.

This is useful when the application needs to ensure that pending log output is pushed out promptly.

## 6. Practical Experiment

Create a ConsoleSink object and write a few messages.

```cpp
#include "logger/console_sink.hpp"

#include <iostream>

int main() {
    logging::ConsoleSink sink;

    sink.write("Application started");
    sink.write("Processing request");
    sink.write("Application finished");

    sink.flush();

    return 0;
}
```

Expected output:

```text
Application started
Processing request
Application finished
```

## 7. Observation

After executing the program:

1. Each message appears in the terminal.
2. Each message is printed on a separate line.
3. The `flush()` operation completes without requiring a return value.
4. ConsoleSink performs output without needing to know anything about Logger's filtering or formatting logic.

## 8. Important Engineering Points

- ConsoleSink implements the LogSink abstraction.
- It follows separation of concerns.
- It uses `std::cout` for standard output.
- `write()` handles one message at a time.
- `flush()` requests delivery of buffered output.
- `override` helps verify correct virtual function overriding.
- ConsoleSink does not own or manage the Logger.
- Thread synchronization is handled at the Logger level in our current design.

**Important:** The current ConsoleSink does not independently synchronize concurrent calls. Its use through our Logger is serialized by the Logger mutex. Direct concurrent use of ConsoleSink outside that Logger is not covered by this guarantee.

## 9. Quick Reference

| Component | Responsibility |
|---|---|
| LogSink | Defines the sink interface |
| ConsoleSink | Implements terminal output |
| std::cout | Standard output stream |
| write() | Writes a message |
| flush() | Requests delivery of buffered output |
| override | Verifies virtual function overriding |

## 10. Progress

**Completed:**
- ConsoleSink header declaration.
- ConsoleSink source implementation.
- Terminal output experiment.
- ConsoleSink unit testing.

**Next Topic:**

`07-file-sink-implementation.md`

We will study file streams, append mode, file opening, error handling, and RAII-based resource management.