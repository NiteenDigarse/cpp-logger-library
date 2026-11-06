# 07. FileSink Implementation

## 1. WHY — Why Do We Need FileSink?

Console output is useful during development, but terminal output is temporary. Once the terminal is closed, previous log messages may no longer be available.

Real-world applications need persistent logs for:

- Debugging application failures.
- Investigating unexpected behavior.
- Monitoring application execution.
- Recording errors and warnings.
- Analyzing application behavior after execution.

To support persistent logging, we implement a component called `FileSink`.

Its responsibility is to write formatted log messages into a file.

Example:

```text
[2026-11-06 11:29:58] [INFO] [Thread: 1] Application started
[2026-11-06 11:29:59] [WARN] [Thread: 1] Low memory warning
[2026-11-06 11:30:00] [ERROR] [Thread: 1] Database connection failed
```

Unlike ConsoleSink, FileSink stores these messages in a file so they can be inspected later.

## 2. Concept — What Is FileSink?

`FileSink` is a concrete implementation of the abstract `LogSink` interface.

It uses the C++ file output stream:

```cpp
std::ofstream
```

`std::ofstream` is provided by the `<fstream>` header and allows a C++ program to write data into files.

Our FileSink provides three important operations:

| Operation | Responsibility |
|---|---|
| Constructor | Opens the log file |
| write() | Writes a log message |
| flush() | Flushes buffered file output |

The Logger does not need to know how file writing works. It only interacts with the common LogSink interface.

## 3. Mental Model — FileSink Architecture

Consider this execution flow:

```text
Application
    |
    v
  Logger
    |
    | Formatted message
    v
  FileSink
    |
    v
 std::ofstream
    |
    v
 Log File
```

FileSink acts as an adapter between our Logger architecture and the C++ file stream system.

The Logger sends a message, and FileSink handles the actual file operation.

## 4. Header File Implementation

File: `include/logger/file_sink.hpp`

```cpp
#pragma once

#include <fstream>
#include <string>

#include "logger/log_sink.hpp"

namespace logging {

class FileSink : public LogSink {
public:
    explicit FileSink(const std::string& filename);

    void write(const std::string& message) override;

    void flush() override;

private:
    std::ofstream file_;
};

}
```

### 4.1 Why Do We Include `<fstream>`?

```cpp
#include <fstream>
```

This header provides file stream classes such as:

- `std::ifstream` — file input.
- `std::ofstream` — file output.
- `std::fstream` — file input and output.

Our FileSink uses `std::ofstream` because its responsibility is writing log messages.

### 4.2 Why Is file_ Private?

```cpp
private:
    std::ofstream file_;
```

The file stream is an internal implementation detail.

External code should not directly manipulate the FileSink's file stream.

Encapsulation protects the internal state and ensures that file operations are performed through the intended interface.

### 4.3 Why Is the Constructor Explicit?

```cpp
explicit FileSink(const std::string& filename);
```

The `explicit` keyword prevents unintended implicit conversions into FileSink objects.

It makes the intention clear: constructing a FileSink requires an explicit filename.

## 5. Source File Implementation

File: `src/file_sink.cpp`

```cpp
#include "logger/file_sink.hpp"

#include <stdexcept>

namespace logging {

FileSink::FileSink(const std::string& filename) {

    file_.open(filename, std::ios::out | std::ios::app);

    if (!file_.is_open()) {
        throw std::runtime_error(
            "Failed to open log file: " + filename
        );
    }
}

void FileSink::write(const std::string& message) {

    file_ << message << '\n';

    if (!file_) {
        throw std::runtime_error(
            "Failed to write to log file"
        );
    }
}

void FileSink::flush() {

    file_.flush();

    if (!file_) {
        throw std::runtime_error(
            "Failed to flush log file"
        );
    }
}

}
```

## 6. Understanding File Opening

### 6.1 The open() Function

```cpp
file_.open(filename, std::ios::out | std::ios::app);
```

This statement opens the requested file for output.

It uses two file-opening flags.

**std::ios::out**

Opens the file for output operations.

**std::ios::app**

Opens the file in append mode.

Append mode ensures that new output is written at the end of the existing file rather than replacing its previous contents.

### 6.2 Understanding the Bitwise OR Operator

```cpp
std::ios::out | std::ios::app
```

The bitwise OR operator combines the two stream flags.

The resulting mode requests output operations with append behavior.

### 6.3 What Happens If the File Does Not Exist?

When opening a regular file with output and append mode, the file is normally created if it does not already exist, provided the operating system permits it.

If the file already exists, previous contents are preserved and new messages are appended.

Example:

Initial file:

```text
Application started
```

After writing another message:

```text
Application started
Application stopped
```

The original message remains intact.

## 7. Understanding File Open Failure

```cpp
if (!file_.is_open()) {
    throw std::runtime_error(
        "Failed to open log file: " + filename
    );
}
```

Opening a file can fail for several reasons:

- The directory does not exist.
- The application does not have sufficient permissions.
- The file system is unavailable.
- The operating system cannot create or access the file.

`is_open()` checks whether the stream has an associated open file.

If opening fails, the constructor throws `std::runtime_error`.

This prevents the application from silently continuing with a FileSink that cannot perform its intended job.

## 8. Understanding write()

```cpp
file_ << message << '\n';
```

The insertion operator writes the supplied message into the file stream.

The newline character separates individual log records.

After writing, we check the stream state:

```cpp
if (!file_) {
    throw std::runtime_error("Failed to write to log file");
}
```

The stream's Boolean state can be used to detect whether an I/O operation has failed.

If the stream is in a failed state, the operation reports an error through an exception.

**Important:** A successful stream-state check does not necessarily mean that the data has reached physical storage. Buffered data may still need to be flushed.

## 9. Understanding flush()

```cpp
file_.flush();
```

File streams can buffer output before passing it to the underlying operating system.

Flushing requests that pending buffered output be delivered to the underlying output sequence.

After flushing, we check the stream state:

```cpp
if (!file_) {
    throw std::runtime_error("Failed to flush log file");
}
```

This allows the application to detect reported output failures.

**Important distinction:** `flush()` does not guarantee that data has been physically committed to the storage device. Durable storage may require operating-system-specific synchronization.

## 10. RAII — Resource Acquisition Is Initialization

One of the most important C++ concepts used in FileSink is RAII.

RAII connects resource lifetime to object lifetime.

In our implementation:

```cpp
private:
    std::ofstream file_;
```

The FileSink owns its file stream as a member object.

### How RAII Works Here

1. A FileSink object is constructed.
2. Its `std::ofstream` member is constructed automatically.
3. The constructor opens the requested file.
4. The FileSink uses the stream during its lifetime.
5. When the FileSink object is destroyed, its `std::ofstream` member is destroyed.
6. The stream destructor automatically closes the associated file if it is still open.

We do not need to manually call `close()` in the FileSink destructor.

### Why Is This Important?

Without RAII, developers might need to manually close resources on every possible execution path.

This becomes difficult when exceptions occur.

RAII makes resource cleanup automatic and helps prevent resource leaks.

## 11. Constructor Failure and Object Lifetime

Consider this code:

```cpp
FileSink sink("missing-folder/application.log");
```

Suppose the directory does not exist.

The constructor attempts to open the file, detects failure, and throws an exception.

The FileSink object is not successfully constructed.

However, its already-constructed member objects are cleaned up automatically during constructor unwinding.

This is an important C++ object lifetime guarantee.

## 12. Practical Experiment

Create a small program to test FileSink independently.

```cpp
#include "logger/file_sink.hpp"

int main() {

    logging::FileSink sink("application.log");

    sink.write("Application started");
    sink.write("Processing request");
    sink.write("Application finished");

    sink.flush();

    return 0;
}
```

Compile and run this program using the project's existing CMake build system or a suitable compiler command.

Expected file content:

```text
Application started
Processing request
Application finished
```

### Experiment 1 — Append Behavior

Run the program again.

Open `application.log`.

Expected content:

```text
Application started
Processing request
Application finished
Application started
Processing request
Application finished
```

The previous contents remain, and new messages are appended.

### Experiment 2 — File Open Failure

Try opening a file inside a directory that does not exist:

```cpp
logging::FileSink sink(
    "missing-folder/application.log"
);
```

Expected behavior:

The constructor throws `std::runtime_error` because the requested file cannot be opened.

## 13. Unit Testing

FileSink has its own test file:

`tests/test_file_sink.cpp`

The tests cover important behaviors:

- Creating and writing to a log file.
- Appending messages.
- Writing multiple messages.
- Handling empty messages.
- Handling special characters.
- Detecting file-opening failure.

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

The FileSink test is registered as:

```cmake
add_test(
    NAME FileSinkTest
    COMMAND test_file_sink
)
```

A passing test suite provides evidence that the tested file-writing behavior works under the tested conditions.

## 14. Important Engineering Points

- FileSink implements the LogSink abstraction.
- `std::ofstream` provides file output functionality.
- `std::ios::out` enables output operations.
- `std::ios::app` preserves existing content and appends new output.
- `is_open()` checks whether the file was opened.
- Stream-state checks help detect reported I/O failures.
- `flush()` requests delivery of buffered output.
- RAII manages the file stream's lifetime.
- The `std::ofstream` destructor automatically closes an open file.
- Constructor failure is reported using `std::runtime_error`.

## 15. Current Design Limitations

Our current FileSink is intentionally simple.

It does not yet implement:

- Log file rotation.
- Maximum file size limits.
- Automatic backup files.
- Date-based log files.
- Configurable buffering policies.
- Explicit disk durability guarantees.

These are possible future improvements, not features of the current implementation.

## 16. Quick Reference

| C++ Feature | Purpose |
|---|---|
| `std::ofstream` | File output stream |
| `<fstream>` | File stream header |
| `open()` | Opens a file |
| `std::ios::out` | Output mode |
| `std::ios::app` | Append mode |
| `is_open()` | Checks whether a file is open |
| `operator<<` | Writes data into a stream |
| `flush()` | Requests delivery of buffered output |
| `std::runtime_error` | Reports runtime I/O errors |
| RAII | Manages resource lifetime automatically |

## 17. Progress

**Completed:**

- FileSink header declaration.
- FileSink source implementation.
- File opening and append behavior.
- File write and flush operations.
- File open and write error checks.
- FileSink unit test suite.
- Integration with the Logger architecture.

**Next Topic:**

`08-logger-class-and-dependency-injection.md`

We will study how Logger coordinates multiple sinks, how constructor injection works, how `std::shared_ptr` manages shared ownership, and how the Logger API is designed.