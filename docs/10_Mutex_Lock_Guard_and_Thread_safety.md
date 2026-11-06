# 10. Mutex, Lock Guard and Thread Safety

## 1. WHY — Why Do We Need Thread Safety?

Modern applications frequently execute multiple tasks concurrently using multiple threads.

A logging system may receive messages from different threads at almost the same time.

Consider an application with three threads:

```text
Thread 1  ---> logger.info("User connected")
Thread 2  ---> logger.error("Database error")
Thread 3  ---> logger.warn("High memory usage")
```

All three threads are accessing the same Logger object.

Without proper synchronization, concurrent operations may interfere with each other.

This can cause:

- Race conditions.
- Data races.
- Corrupted shared state.
- Unpredictable application behavior.
- Incorrect log output.

To protect shared state, our Logger uses synchronization mechanisms provided by the C++ standard library.

## 2. Concept — What Is a Mutex?

A mutex stands for **Mutual Exclusion**.

It is a synchronization primitive that allows only one thread at a time to own the mutex.

In C++, `std::mutex` is available through:

```cpp
#include <mutex>
```

Basic example:

```cpp
std::mutex mutex_;

mutex_.lock();

// Critical section

mutex_.unlock();
```

When one thread successfully locks the mutex, another thread attempting to lock the same mutex must wait until it becomes available.

This helps protect operations that access shared mutable state.

## 3. Mental Model — A Shared Room

Imagine three people trying to enter one room.

Only one person is allowed inside at a time.

```text
       Thread 1
           |
           v
       +-------+
       | Mutex |
       +-------+
           |
       Critical Section
           |
        Unlock
           |
      +----+----+
      |         |
   Thread 2  Thread 3
   Waiting   Waiting
```

The mutex acts like a lock on the room.

A thread must acquire the lock before entering the protected critical section.

After the current thread releases the lock, another waiting thread can acquire it.

## 4. What Is a Critical Section?

A critical section is a portion of code that accesses shared resources and must be protected from conflicting concurrent operations.

In our Logger, important shared state includes:

```cpp
std::vector<std::shared_ptr<LogSink>> sinks_;
LogLevel min_level_;
```

The Logger's mutex is used to synchronize access to this state and serialize its logging operations.

For example:

```cpp
void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}
```

The assignment to `min_level_` happens while the mutex is held.

## 5. Understanding std::lock_guard

Manually calling `lock()` and `unlock()` can be dangerous.

Consider:

```cpp
mutex_.lock();

performOperation();

mutex_.unlock();
```

What happens if `performOperation()` throws an exception?

The execution may leave the function before reaching `unlock()`.

The mutex can remain locked, causing other threads to wait indefinitely.

C++ provides `std::lock_guard` to manage this automatically.

```cpp
std::lock_guard<std::mutex> lock(mutex_);
```

### How Does It Work?

1. The lock_guard object is constructed.
2. Its constructor acquires the mutex.
3. The current thread executes the protected code.
4. The lock_guard object reaches the end of its scope.
5. Its destructor automatically releases the mutex.

This follows the RAII principle.

### Mental Model

```text
Function Entry
     |
     v
Construct lock_guard
     |
     v
Acquire Mutex
     |
     v
Critical Section
     |
     v
Function Exit
     |
     v
Destroy lock_guard
     |
     v
Release Mutex
```

The mutex is released even when an exception causes stack unwinding.

## 6. Logger Mutex Declaration

File: `include/logger/logger.hpp`

```cpp
private:
    std::vector<std::shared_ptr<LogSink>> sinks_;

    LogLevel min_level_;

    mutable std::mutex mutex_;
```

### Why Is mutex_ Private?

External code should not directly control the Logger's internal synchronization.

Keeping it private allows Logger to manage its own thread-safety policy.

### Why Is mutex_ Mutable?

Our `getLevel()` function is const:

```cpp
LogLevel getLevel() const;
```

A const member function normally cannot modify non-mutable data members.

However, locking the mutex is an internal synchronization operation. It does not logically change the Logger's configuration.

Therefore, we declare:

```cpp
mutable std::mutex mutex_;
```

This allows the mutex to be locked inside const member functions.

## 7. Applying Mutex in Logger

Our implementation uses the same mutex in four functions.

### 7.1 log()

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

The mutex protects the logging operation from concurrent operations through the same Logger instance.

The lock remains held while the message is filtered, formatted, and sent to all configured sinks.

### 7.2 setLevel()

```cpp
void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}
```

This prevents an unsynchronized update of the minimum log level while another Logger operation reads it.

### 7.3 getLevel()

```cpp
LogLevel Logger::getLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return min_level_;
}
```

This protects the read of the shared configuration variable.

### 7.4 flush()

```cpp
void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& sink : sinks_) {
        sink->flush();
    }
}
```

This serializes flush operations with logging operations through the same Logger.

## 8. Understanding Race Conditions

A race condition occurs when program behavior depends on the unpredictable timing or ordering of concurrent operations.

Consider an unprotected shared counter:

```cpp
int counter = 0;
```

Two threads execute:

```cpp
counter++;
```

The increment operation is not necessarily one indivisible machine operation.

Conceptually, it involves:

1. Reading the current value.
2. Increasing the value.
3. Writing the new value.

Two threads can interfere with these steps.

In C++, an unsynchronized conflicting access to a non-atomic shared object, where at least one access is a write, can constitute a data race and cause undefined behavior.

A mutex can protect this operation:

```cpp
std::mutex mutex_;
int counter = 0;

void increment() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++counter;
}
```

Both threads must acquire the same mutex before modifying the counter.

## 9. Mutex and Memory Synchronization

A mutex does more than prevent two threads from executing a critical section simultaneously.

Successful mutex unlock and subsequent successful lock operations establish synchronization between threads.

This provides the memory-ordering guarantees needed for protected shared state.

In practical terms, changes made by one thread while holding the mutex become visible to a thread that subsequently acquires the same mutex.

This is one reason all participating threads must use the same synchronization mechanism.

## 10. Practical Experiment — Concurrent Logger Access

Our Logger test suite includes a multithreaded logging test.

The test creates multiple threads that use the same Logger instance.

Conceptually:

```cpp
std::vector<std::thread> threads;

for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&logger]() {
        for (int j = 0; j < 100; ++j) {
            logger.info("Concurrent message");
        }
    });
}

for (auto& thread : threads) {
    thread.join();
}
```

### Expected Message Count

```text
Number of threads = 4
Messages per thread = 100

Total messages = 4 * 100

Total = 400
```

The test verifies that all 400 messages reach the test sink.

### Why Do We Use join()?

```cpp
thread.join();
```

The main thread waits until each worker thread finishes execution.

This ensures the test does not inspect the final message count before all logging operations have completed.

## 11. Understanding TestSink Synchronization

Our Logger tests use a custom TestSink.

The test sink maintains a collection of received messages.

Because multiple test operations may access its internal state, the TestSink also uses its own mutex.

Conceptually:

```cpp
class TestSink : public LogSink {
private:
    mutable std::mutex mutex_;
    std::vector<std::string> messages_;

public:
    void write(const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back(message);
    }
};
```

There are two different synchronization responsibilities:

| Mutex | Responsibility |
|---|---|
| Logger mutex | Serializes operations through the Logger |
| TestSink mutex | Protects the TestSink's internal message collection |

This distinction is important when designing thread-safe components.

## 12. Important Design Trade-off — Locking While Calling Sinks

Our current Logger keeps the mutex locked while invoking:

```cpp
sink->write(formatted_message);
```

This is simple and ensures serialized sink calls through the Logger.

However, it introduces some limitations.

### 12.1 Slow Sink

If FileSink or another custom sink is slow, other threads must wait for the Logger mutex.

### 12.2 Exception From a Sink

If a sink throws an exception, the logging operation is interrupted.

The lock_guard still releases the mutex during exception unwinding.

However, subsequent sinks may not receive that message.

### 12.3 Reentrant Logging

If a custom sink calls the same Logger again while the Logger mutex is already held, the thread may attempt to lock the same non-recursive mutex again.

This can cause a deadlock.

These are important design trade-offs that we will review during the final engineering audit.

## 13. std::mutex vs std::recursive_mutex

C++ provides different mutex types.

| Feature | std::mutex | std::recursive_mutex |
|---|---|---|
| Repeated locking by same thread | Not allowed | Supported |
| Typical use | Normal synchronization | Recursive locking scenarios |
| Complexity | Simpler | More complex |
| Our Logger | Used | Not used |

Using `std::recursive_mutex` is not automatically a better solution to reentrant logging.

It can hide design problems and still leave other recursion or consistency issues.

Our current Logger intentionally uses `std::mutex`.

## 14. Current Thread-Safety Guarantees

For calls made through the same Logger instance:

- `log()` is synchronized.
- `setLevel()` is synchronized.
- `getLevel()` is synchronized.
- `flush()` is synchronized.
- Concurrent logging operations are serialized while the Logger mutex is held.

However, these guarantees do not automatically make every external object thread-safe.

For example, directly calling a sink from another thread outside the Logger does not use the Logger's mutex.

Thread safety must be considered at every shared component boundary.

## 15. Important Engineering Points

- A mutex provides mutual exclusion.
- A critical section contains protected shared-state operations.
- `std::lock_guard` provides automatic lock management.
- RAII ensures the lock is released when the guard is destroyed.
- A mutex protects against unsynchronized conflicting access when all participants follow the same locking protocol.
- The same mutex is used by Logger's logging and configuration operations.
- The TestSink has a separate mutex for its internal state.
- `std::mutex` is non-recursive.
- Holding a lock while calling external code has performance and deadlock implications.
- Thread safety of one object does not automatically guarantee thread safety of all connected objects.

## 16. Quick Reference

| C++ Feature | Purpose |
|---|---|
| `std::mutex` | Mutual exclusion |
| `lock()` | Acquires a mutex |
| `unlock()` | Releases a mutex |
| `std::lock_guard` | Automatic lock management |
| `mutable` | Allows internal synchronization in const methods |
| Critical Section | Protected section of code |
| Race Condition | Behavior affected by concurrent operation ordering |
| Data Race | Conflicting unsynchronized memory accesses |
| `std::thread` | Creates a thread |
| `join()` | Waits for a thread to finish |

## 17. Progress

**Completed:**

- Mutex declaration in Logger.
- Lock guard implementation.
- Synchronization of logging operations.
- Synchronization of level configuration.
- Synchronization of flush operations.
- Multithreaded Logger testing.
- TestSink synchronization.
- Understanding of critical sections and RAII-based locking.

**Next Topic:**

`11-raii-ownership-and-exception-safety.md`

We will study resource ownership, object lifetime, constructor failure, exception propagation, and how RAII helps build reliable C++ components.