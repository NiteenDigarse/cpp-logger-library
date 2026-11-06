# 15. Performance Benchmarking and Analysis

## 1. WHY — Why Performance Benchmarking?

A logging system is frequently used throughout an application's execution.

If logging introduces excessive overhead, it can negatively affect application performance.

Potential performance costs include:

- Mutex acquisition and contention.
- LogRecord construction.
- Timestamp generation.
- Thread ID retrieval.
- String formatting.
- Sink dispatch.
- Memory allocation.
- Console or file I/O.

Benchmarking helps us measure the behavior of the current implementation instead of making assumptions about its performance.

Our Logger uses synchronous execution and mutex-based synchronization.

The objective of this benchmark is to understand how its throughput changes under different workloads.

## 2. Benchmark Objectives

The benchmark evaluates five scenarios:

1. Single-thread enabled logging.
2. Two-thread enabled logging.
3. Four-thread enabled logging.
4. Eight-thread enabled logging.
5. Four-thread filtered logging.

The experiment uses an in-memory CountingSink to avoid terminal and file I/O dominating the measurements.

The benchmark measures:

- Total elapsed time.
- Throughput in operations per second.
- Number of accepted messages.
- Median throughput across repeated runs.

## 3. Performance Metrics

### 3.1 Throughput

Throughput measures the number of operations completed per unit of time.

\[
Throughput = \frac{Total\ Operations}{Elapsed\ Time}
\]

For example, if 100,000 operations complete in 2 seconds:

\[
Throughput = 50,000\ operations/second
\]

### 3.2 Latency

Latency measures the elapsed time for an individual operation.

Our current benchmark does not collect individual-operation latency or p50, p95 and p99 percentiles.

### 3.3 Median Throughput

The benchmark performs five repetitions for each scenario.

It sorts the five throughput measurements and selects the middle value.

For five measurements:

\[
Median = SortedResults[2]
\]

The median reduces the influence of unusually high or low individual runs.

## 4. Why std::chrono::steady_clock?

The benchmark uses:

```cpp
std::chrono::steady_clock
```

This clock is designed to be monotonic and is appropriate for measuring elapsed time.

Example:

```cpp
auto start = std::chrono::steady_clock::now();

// Workload

auto end = std::chrono::steady_clock::now();

double elapsed =
    std::chrono::duration<double>(end - start).count();
```

`system_clock` is used for wall-clock timestamps in log records, while `steady_clock` is used for elapsed-time measurement.

## 5. Benchmark Architecture

```text
              Benchmark Runner
                     |
                     v
               Create Logger
                     |
                     v
                CountingSink
                     |
                     v
                Warm-up Phase
                     |
                     v
              Reset Sink Counter
                     |
                     v
              Create Worker Threads
                     |
                     v
               Start Signal
                     |
                     v
             Execute Logging Calls
                     |
                     v
             Logger Synchronization
                     |
                     v
              LogRecord Formatting
                     |
                     v
                CountingSink
                     |
                     v
              Join Worker Threads
                     |
                     v
             Calculate Throughput
                     |
                     v
              Verify Message Count
```

## 6. CountingSink

The benchmark uses a custom in-memory sink.

```cpp
class CountingSink final : public LogSink {
public:
    void write(const std::string&) override {
        message_count_.fetch_add(
            1,
            std::memory_order_relaxed
        );
    }

    void flush() override {}

    std::size_t count() const {
        return message_count_.load(
            std::memory_order_relaxed
        );
    }

    void reset() {
        message_count_.store(
            0,
            std::memory_order_relaxed
        );
    }

private:
    std::atomic<std::size_t> message_count_{0};
};
```

The atomic counter allows multiple threads to update the accepted-message count safely.

`memory_order_relaxed` provides atomicity for the counter without imposing additional synchronization ordering.

The counter is used for measurement verification, not for coordinating the worker threads.

### Measurement Limitation

The benchmark measures Logger execution together with CountingSink's atomic increment.

Therefore, results represent the combined workload, not isolated Logger overhead.

## 7. Benchmark Configuration

The experiment uses the following configuration:

| Parameter | Value |
|---|---|
| Language standard | C++17 |
| Compiler | GNU 16.2.0 |
| Compiler path | MSYS2 UCRT64 |
| Build configuration | Release |
| Messages per thread | 100,000 |
| Repetitions per scenario | 5 |
| Warm-up messages per thread | 1,000 |
| Clock | `std::chrono::steady_clock` |
| Sink | In-memory CountingSink |
| Synchronization | `std::mutex` |

The benchmark executable was built using:

```bash
cmake -S . -B build-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

cmake --build build-release --target logger_benchmark
```

Execution:

```bash
.\build-release\logger_benchmark.exe
```

## 8. Actual Benchmark Results

The following values were collected from the Release benchmark execution.

### 8.1 Single-thread Enabled Logging

| Run | Elapsed Time (s) | Throughput (ops/sec) |
|---|---:|---:|
| 1 | 1.275876 | 78,377.52 |
| 2 | 1.298188 | 77,030.46 |
| 3 | 1.301505 | 76,834.11 |
| 4 | 1.294777 | 77,233.36 |
| 5 | 1.275647 | 78,391.57 |

**Median throughput: 77,233.36 operations/sec**

Accepted messages: 100,000.

Verification: PASSED.

### 8.2 Two-thread Enabled Logging

| Run | Elapsed Time (s) | Throughput (ops/sec) |
|---|---:|---:|
| 1 | 2.941456 | 67,993.55 |
| 2 | 2.963388 | 67,490.32 |
| 3 | 3.032923 | 65,942.98 |
| 4 | 2.904953 | 68,847.93 |
| 5 | 2.963016 | 67,498.79 |

**Median throughput: 67,498.79 operations/sec**

Accepted messages: 200,000.

Verification: PASSED.

### 8.3 Four-thread Enabled Logging

| Run | Elapsed Time (s) | Throughput (ops/sec) |
|---|---:|---:|
| 1 | 5.986300 | 66,819.23 |
| 2 | 5.965661 | 67,050.41 |
| 3 | 5.947913 | 67,250.48 |
| 4 | 5.903671 | 67,754.45 |
| 5 | 6.001703 | 66,647.74 |

**Median throughput: 67,050.41 operations/sec**

Accepted messages: 400,000.

Verification: PASSED.

### 8.4 Eight-thread Enabled Logging

| Run | Elapsed Time (s) | Throughput (ops/sec) |
|---|---:|---:|
| 1 | 11.957444 | 66,903.93 |
| 2 | 12.013652 | 66,590.91 |
| 3 | 12.613662 | 63,423.30 |
| 4 | 15.814949 | 50,585.05 |
| 5 | 15.600841 | 51,279.29 |

**Median throughput: 63,423.30 operations/sec**

Accepted messages: 800,000.

Verification: PASSED.

### 8.5 Four-thread Filtered Logging

Configuration:

- Threads: 4.
- Messages per thread: 100,000.
- Minimum level: ERROR.
- Submitted message level: INFO.

Because INFO is below ERROR, every message should be rejected.

| Run | Elapsed Time (s) | Throughput (attempted calls/sec) |
|---|---:|---:|
| 1 | 0.023608 | 16,943,552.55 |
| 2 | 0.022921 | 17,451,017.18 |
| 3 | 0.029769 | 13,436,706.39 |
| 4 | 0.034402 | 11,627,197.18 |
| 5 | 0.026222 | 15,254,541.09 |

**Median throughput: 15,254,541.09 attempted calls/sec**

Accepted messages: 0.

Verification: PASSED.

The filtered result represents attempted logging calls per second, not accepted messages per second.

## 9. Combined Results

| Scenario | Threads | Total Calls | Median Throughput |
|---|---:|---:|---:|
| Enabled | 1 | 100,000 | 77,233.36 ops/sec |
| Enabled | 2 | 200,000 | 67,498.79 ops/sec |
| Enabled | 4 | 400,000 | 67,050.41 ops/sec |
| Enabled | 8 | 800,000 | 63,423.30 ops/sec |
| Filtered | 4 | 400,000 | 15,254,541.09 attempted calls/sec |

## 10. Throughput Analysis

The enabled-logging benchmark shows the following trend:

- Single-thread execution produced the highest median throughput.
- Two-thread execution produced lower throughput than single-thread execution.
- Four-thread throughput remained close to the two-thread result.
- Eight-thread throughput was lower and showed greater variation between runs.

The results do not show proportional throughput scaling with additional threads.

### Why Might This Happen?

The current Logger uses one mutex to synchronize logging operations.

The critical section includes:

1. Level filtering.
2. LogRecord creation.
3. Timestamp retrieval.
4. Thread ID retrieval.
5. Message formatting.
6. Sink writes.

Only one thread can hold this mutex at a time.

Consequently, concurrent callers can spend time waiting for mutex ownership.

This is consistent with the observed lack of throughput scaling.

However, the benchmark alone does not isolate mutex contention as the only cause. Scheduling, CPU activity and benchmark overhead can also influence the results.

## 11. Filtering Analysis

The filtered scenario achieved a much higher attempted-call rate than enabled logging.

This is expected because rejected messages skip:

- LogRecord construction.
- Timestamp retrieval for the record.
- Thread ID retrieval for the record.
- Message formatting.
- Sink writing.

The Logger still performs synchronization and the severity-level comparison.

The result demonstrates the performance benefit of filtering before expensive logging work.

## 12. Benchmark Limitations

The results should be interpreted within the scope of the experiment.

### 12.1 CountingSink Overhead

CountingSink performs an atomic increment for every accepted message.

This contributes to the measured execution time.

### 12.2 Thread Scheduling

Worker threads are created before timing starts, but the measured interval includes the start signal, worker scheduling and thread joining.

### 12.3 Run-to-Run Variation

The eight-thread benchmark shows noticeable variation in its last two runs.

Possible contributors include CPU scheduling, background processes, system load and synchronization contention.

The exact cause cannot be established from this benchmark alone.

### 12.4 Latency Not Measured

The benchmark does not measure individual-call latency or latency percentiles.

### 12.5 No Console or File I/O Comparison

The experiment uses an in-memory sink.

ConsoleSink and FileSink performance require separate benchmarks with clearly defined buffering and flushing behavior.

### 12.6 No Sanitizer or Profiling Evidence

This benchmark does not prove the absence of data races, memory errors or other correctness defects.

Those require separate validation.

## 13. Future Performance Experiments

Potential future experiments include:

- ConsoleSink throughput.
- FileSink throughput.
- Different message sizes.
- Different numbers of sinks.
- Individual logging-call latency.
- p50, p95 and p99 latency.
- CPU profiling.
- Memory allocation profiling.
- Alternative synchronization designs.
- Asynchronous logging.

Any optimization should be supported by a repeatable before-and-after measurement.

## 14. Important Engineering Points

- Throughput measures operations completed per unit of time.
- Latency measures the duration of an individual operation.
- `steady_clock` is appropriate for elapsed-time measurements.
- Release builds are preferred for performance experiments.
- Warm-up operations should be excluded from timed measurements.
- Repeated runs help identify variability.
- Median throughput is useful when measurements fluctuate.
- A mutex can limit concurrent throughput.
- Filtering before formatting avoids unnecessary work.
- Sink implementation affects benchmark results.
- Benchmark conclusions must remain within the experiment's scope.
- Performance improvements should be based on measured evidence.

## 15. Final Benchmark Status

**Build status:** Successful.

**Benchmark status:** Completed.

**Scenarios:** 5.

**Repetitions:** 25 total.

**Message verification:** All scenarios passed.

**Release benchmark executable:** `logger_benchmark.exe`

**Actual performance results:** Recorded in this document.

**Limitations:** No latency percentile measurements, no ConsoleSink/FileSink comparison and no CPU profiling.

## 16. Quick Reference

| Term | Meaning |
|---|---|
| Benchmark | Controlled performance experiment |
| Throughput | Operations completed per second |
| Latency | Time taken by an operation |
| Median | Middle value of sorted measurements |
| Warm-up | Operations performed before measurement |
| Mutex Contention | Competition for mutex ownership |
| CountingSink | In-memory sink used to count accepted messages |
| Release Build | Optimized build configuration |
| Filtering | Rejecting messages below the configured level |

## 17. Progress

**Completed:**

- Benchmark implementation.
- Release build.
- Five benchmark scenarios.
- Five repetitions per scenario.
- Throughput measurements.
- Message-count verification.
- Median calculation.
- Performance analysis.
- Actual results documentation.

**Next Topic:**

`16-logger-final-engineering-audit.md`

The final audit will review the Logger architecture, implementation quality, tests, performance evidence, known limitations and future improvements.