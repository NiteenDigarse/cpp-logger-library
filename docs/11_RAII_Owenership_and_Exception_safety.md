# 11. RAII, Ownership and Exception Safety

## 1. WHY — Why Do We Need Resource Management?

C++ allows developers to manage resources such as:

- Dynamically allocated memory.
- Files.
- Mutex locks.
- Network connections.
- Operating system handles.

However, resources must be released correctly when they are no longer required.

Consider a file opened by an application.

```cpp
std::ofstream file("application.log");
```

If the application exits a function unexpectedly, we still need to ensure that the file resource is released.

Manually managing every resource becomes difficult, especially when exceptions occur.

C++ provides a powerful programming technique called **RAII (Resource Acquisition Is Initialization)**.

RAII connects resource ownership to object lifetime.

Our Logger uses RAII in several places:

- `std::ofstream` manages file resources.
- `std::shared_ptr` manages shared object ownership.
- `std::lock_guard` manages mutex locking.
- Class member objects are destroyed automatically when their owner is destroyed.

## 2. Concept — What Is RAII?

RAII stands for:

**Resource Acquisition Is Initialization.**

The main idea is:

1. Acquire a resource during object construction or initialization.
2. Store ownership of that resource inside an object.
3. Release the resource automatically when the object is destroyed.

The object's lifetime controls the resource's lifetime.

### Mental Model

```text
       Object Construction
               |
               v
        Resource Acquired
               |
               v
          Object Lifetime
               |
               v
        Object Destruction
               |
               v
        Resource Released
```

This makes resource management predictable and reduces the need for manual cleanup.

## 3. RAII in Our FileSink

File: `include/logger/file_sink.hpp`

```cpp
class FileSink : public LogSink {
public:
    explicit FileSink(const std::string& filename);

    void write(const std::string& message) override;
    void flush() override;

private:
    std::ofstream file_;
};
```

The important member is:

```cpp
std::ofstream file_;
```

FileSink contains an `std::ofstream` object.

When FileSink is constructed, its file stream member is also constructed.

Our constructor opens the requested file:

```cpp
FileSink::FileSink(const std::string& filename) {
    file_.open(filename, std::ios::out | std::ios::app);

    if (!file_.is_open()) {
        throw std::runtime_error(
            "Failed to open log file: " + filename
        );
    }
}
```

When FileSink is destroyed, its `std::ofstream` member is destroyed automatically.

The stream destructor closes the associated file if it is still open.

We do not need to write a custom FileSink destructor just to close this file.

## 4. Why Is RAII Better Than Manual Resource Management?

Consider manual resource management:

```cpp
void writeLog() {
    std::ofstream file("application.log");

    file << "Application started";

    file.close();
}
```

This simple example works, but manual cleanup becomes harder when there are multiple return paths or exceptions.

Consider:

```cpp
void writeLog() {
    std::ofstream file("application.log");

    if (!file.is_open()) {
        return;
    }

    performOperation();

    file.close();
}
```

If `performOperation()` throws an exception, the explicit `close()` statement is skipped.

With RAII, the stream object is destroyed automatically during stack unwinding.

```cpp
void writeLog() {
    std::ofstream file("application.log");

    if (!file.is_open()) {
        return;
    }

    performOperation();

} // file destructor handles cleanup
```

The same principle applies to many C++ resource-owning types.

## 5. Object Lifetime and Constructor Failure

An important C++ rule is that a fully constructed object has a complete lifetime, but an object whose constructor throws is not successfully created.

Consider our FileSink:

```cpp
FileSink::FileSink(const std::string& filename) {
    file_.open(filename, std::ios::out | std::ios::app);

    if (!file_.is_open()) {
        throw std::runtime_error(
            "Failed to open log file: " + filename
        );
    }
}
```

Suppose the directory does not exist.

The following happens:

1. The `file_` member is constructed.
2. The constructor attempts to open the file.
3. The opening operation fails.
4. The constructor throws `std::runtime_error`.
5. The partially constructed FileSink object is not completed.
6. Already-constructed members are destroyed automatically.

The `std::ofstream` member therefore gets its normal cleanup opportunity.

**Important:** The destructor of the FileSink object itself is not called when its constructor fails. Its fully constructed members and base classes are destroyed.

## 6. Resource Ownership in Logger

Our Logger stores sinks using:

```cpp
std::vector<std::shared_ptr<LogSink>> sinks_;
```

This introduces another important concept: ownership.

Ownership answers the question:

**Which object is responsible for keeping a resource alive and eventually releasing it?**

Our design uses shared ownership for sink objects.

### 6.1 What Is std::shared_ptr?

`std::shared_ptr` is a smart pointer that manages an object through shared ownership.

Multiple shared pointers can refer to the same object.

The managed object is destroyed when its last owning shared pointer is destroyed or reset.

Example:

```cpp
auto console =
    std::make_shared<ConsoleSink>();

std::shared_ptr<LogSink> sink = console;
```

Both pointers share ownership of the same ConsoleSink object.

### 6.2 Ownership Model

```text
       console
          |
          v
    +-------------+
    | ConsoleSink |
    +-------------+
          ^
          |
         sink
```

Both shared pointers contribute to the ownership count.

Destroying one pointer does not destroy the object while another owning pointer remains.

## 7. Why Does Logger Use shared_ptr?

Our Logger receives sinks from external application code.

Example:

```cpp
auto console_sink =
    std::make_shared<logging::ConsoleSink>();

auto file_sink =
    std::make_shared<logging::FileSink>("application.log");

std::vector<std::shared_ptr<logging::LogSink>> sinks;

sinks.push_back(console_sink);
sinks.push_back(file_sink);

logging::Logger logger(std::move(sinks));
```

The application retains its own shared pointers, and Logger stores shared pointers to the same sinks.

This is useful when both the application and Logger need shared ownership.

However, shared ownership is not always necessary.

If Logger were designed to be the exclusive owner of all sinks, `std::unique_ptr` could be a better choice.

The ownership model should match the actual requirements of the system.

## 8. Understanding std::move and Ownership Transfer

Our Logger constructor accepts:

```cpp
explicit Logger(
    std::vector<std::shared_ptr<LogSink>> sinks,
    LogLevel min_level = LogLevel::TRACE
);
```

Its implementation uses:

```cpp
Logger::Logger(
    std::vector<std::shared_ptr<LogSink>> sinks,
    LogLevel min_level
)
    : sinks_(std::move(sinks)),
      min_level_(min_level)
{
    // Validation
}
```

`std::move` converts its argument into an rvalue expression.

This enables the vector's move constructor to transfer its internal resources into `sinks_`.

It avoids copying every vector element during this transfer.

**Important:** `std::move` does not itself transfer ownership or move data. The selected move operation performs the actual transfer.

## 9. RAII in std::lock_guard

Our Logger uses:

```cpp
std::lock_guard<std::mutex> lock(mutex_);
```

The lock_guard object acquires the mutex during construction.

When the guard is destroyed, it automatically releases the mutex.

Example:

```cpp
void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);

    min_level_ = level;

}
```

The mutex is released automatically when the function scope ends.

This also works when an exception is thrown while the guard is alive.

Without RAII, manually unlocking a mutex on every possible execution path would be error-prone.

## 10. Concept — What Is Exception Safety?

Exception safety describes how a component behaves when an operation fails by throwing an exception.

A well-designed C++ component should maintain valid state and manage its resources correctly when failures occur.

The standard exception-safety guarantees are commonly described in three levels.

### 10.1 No-Throw Guarantee

An operation guarantees that it will not throw exceptions.

Examples include many destructors and certain move operations.

A destructor used for RAII cleanup should generally not allow exceptions to escape.

### 10.2 Strong Exception Guarantee

An operation either:

- Completes successfully, or
- Fails without changing the observable state of the object.

This is often described as the commit-or-rollback guarantee.

### 10.3 Basic Exception Guarantee

An operation may fail and change the object's state, but the object remains valid and its resources are not leaked.

### Comparison

| Guarantee | Meaning |
|---|---|
| No-Throw | Operation does not throw |
| Strong | Failure leaves the original state unchanged |
| Basic | Object remains valid after failure |

These guarantees are useful when designing containers, memory managers, file handlers, and other C++ components.

## 11. Exception Handling in Our FileSink

Our constructor throws an exception when the file cannot be opened.

```cpp
if (!file_.is_open()) {
    throw std::runtime_error(
        "Failed to open log file: " + filename
    );
}
```

This makes initialization failure explicit.

The caller can handle it:

```cpp
try {
    logging::FileSink sink("missing-folder/app.log");
}
catch (const std::runtime_error& error) {
    std::cerr << error.what() << '\n';
}
```

Expected output:

```text
Failed to open log file: missing-folder/app.log
```

This example demonstrates how a resource acquisition failure can be reported through an exception.

## 12. Exception Propagation in Logger

Our Logger calls every configured sink:

```cpp
for (const auto& sink : sinks_) {
    sink->write(formatted_message);
}
```

Suppose the first sink throws an exception.

The exception propagates out of `Logger::log()`.

Consequences:

- The current logging operation is interrupted.
- Subsequent sinks are not called for that message.
- The Logger mutex is released during stack unwinding because it is managed by lock_guard.
- The caller receives the exception unless it is caught at a higher level.

Our current Logger does not catch individual sink exceptions or implement a fallback policy.

This is an intentional area for future design review.

## 13. Exception Safety and Mutex Management

Consider:

```cpp
void Logger::log(
    LogLevel level,
    const std::string& message
) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Logging operations
}
```

Suppose a sink throws an exception.

The execution leaves the function through exception propagation.

However, the lock_guard object is still destroyed automatically.

Its destructor releases the mutex.

This prevents the mutex from remaining permanently locked because of an exception in the logging operation.

This is one of the most important practical benefits of combining RAII with exception handling.

## 14. Practical Experiment

### Experiment 1 — FileSink Resource Cleanup

Create a FileSink:

```cpp
{
    logging::FileSink sink("application.log");

    sink.write("Testing RAII");
    sink.flush();

} // FileSink and its file stream are destroyed here
```

Observe:

1. The file is opened.
2. The message is written.
3. The buffered output is flushed.
4. The FileSink object leaves scope.
5. Its file stream member is destroyed and the open file is closed.

### Experiment 2 — Constructor Failure

Try constructing a FileSink with an invalid directory:

```cpp
try {
    logging::FileSink sink(
        "missing-folder/application.log"
    );
}
catch (const std::exception& error) {
    std::cout << error.what() << '\n';
}
```

Observe that the constructor reports the failure instead of producing a usable FileSink object.

### Experiment 3 — Mutex Cleanup During Exception

Conceptually, if an exception occurs while a lock_guard is alive, stack unwinding destroys the guard and releases the mutex.

This behavior is guaranteed by RAII.

## 15. Important Engineering Points

- RAII connects resource lifetime to object lifetime.
- Resource acquisition commonly occurs during object initialization.
- Resource cleanup happens automatically during destruction.
- `std::ofstream` manages file stream resources.
- `std::shared_ptr` manages shared object ownership.
- `std::move` enables move operations.
- `std::lock_guard` manages mutex ownership.
- Constructor failure destroys already-constructed members.
- Exception propagation does not bypass RAII cleanup.
- A destructor should not allow exceptions to escape.
- Exception-safety guarantees describe how objects behave during failure.
- Our current Logger propagates sink exceptions.
- The Logger does not currently isolate failures between individual sinks.

## 16. Quick Reference

| C++ Feature | Purpose |
|---|---|
| RAII | Resource lifetime management |
| Constructor | Initializes object state and resources |
| Destructor | Releases owned resources |
| `std::ofstream` | Manages file stream resources |
| `std::shared_ptr` | Shared ownership |
| `std::unique_ptr` | Exclusive ownership |
| `std::move` | Enables move semantics |
| `std::lock_guard` | Automatic mutex management |
| `std::runtime_error` | Reports runtime failures |
| Stack unwinding | Cleans up local objects during exception propagation |
| Strong Guarantee | Failure leaves observable state unchanged |
| Basic Guarantee | Object remains valid after failure |

## 17. Progress

**Completed:**

- Understanding RAII.
- Resource lifetime management.
- FileSink resource cleanup.
- Constructor failure and member destruction.
- Shared ownership with shared_ptr.
- Move semantics in Logger construction.
- RAII-based mutex management.
- Exception propagation.
- Exception-safety guarantees.

**Next Topic:**

`12-unit-testing-and-test-design.md`

We will study how our Logger components are tested, how test cases are structured, how failures are detected, and how to design meaningful unit tests.