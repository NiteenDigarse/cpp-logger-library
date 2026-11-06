
#include "logger/logger.hpp"
#include "logger/log_sink.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace logging;

class CountingSink final : public LogSink {
public:
    void write(const std::string&) override {
        message_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void flush() override {}

    std::size_t count() const {
        return message_count_.load(std::memory_order_relaxed);
    }

    void reset() {
        message_count_.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<std::size_t> message_count_{0};
};

struct BenchmarkConfig {
    std::string name;
    std::size_t thread_count;
    std::size_t messages_per_thread;
    LogLevel minimum_level;
    LogLevel message_level;
};

struct BenchmarkResult {
    double elapsed_seconds;
    double operations_per_second;
    std::size_t accepted_messages;
};

BenchmarkResult run_once(const BenchmarkConfig& config) {
    auto sink = std::make_shared<CountingSink>();

    std::vector<std::shared_ptr<LogSink>> sinks{sink};

    Logger logger(sinks, config.minimum_level);

    constexpr std::size_t warmup_messages = 1000;

    // Warm-up: excluded from timed measurement.
    for (std::size_t t = 0; t < config.thread_count; ++t) {
        for (std::size_t i = 0; i < warmup_messages; ++i) {
            logger.log(config.message_level, "Warm-up message");
        }
    }

    // Do not include warm-up messages in the result.
    sink->reset();

    std::atomic<bool> start{false};
    std::vector<std::thread> workers;

    workers.reserve(config.thread_count);

    for (std::size_t t = 0; t < config.thread_count; ++t) {
        workers.emplace_back([&logger, &config, &start]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (std::size_t i = 0;
                 i < config.messages_per_thread;
                 ++i) {
                logger.log(
                    config.message_level,
                    "Benchmark logging message"
                );
            }
        });
    }

    const auto start_time = std::chrono::steady_clock::now();

    start.store(true, std::memory_order_release);

    for (auto& worker : workers) {
        worker.join();
    }

    const auto end_time = std::chrono::steady_clock::now();

    const double elapsed_seconds =
        std::chrono::duration<double>(
            end_time - start_time
        ).count();

    const std::size_t total_operations =
        config.thread_count * config.messages_per_thread;

    const std::size_t accepted_messages = sink->count();

    if (elapsed_seconds <= 0.0) {
        throw std::runtime_error("Invalid benchmark duration");
    }

    const double operations_per_second =
        static_cast<double>(total_operations) / elapsed_seconds;

    return {
        elapsed_seconds,
        operations_per_second,
        accepted_messages
    };
}

void run_benchmark(const BenchmarkConfig& config) {
    constexpr int repetitions = 5;

    std::vector<double> throughput_results;
    throughput_results.reserve(repetitions);

    std::size_t expected_accepted = 0;

    const std::size_t total_operations =
        config.thread_count * config.messages_per_thread;

    if (config.message_level >= config.minimum_level) {
        expected_accepted = total_operations;
    }

    std::cout << "\n========================================\n";
    std::cout << "Benchmark: " << config.name << '\n';
    std::cout << "Threads: " << config.thread_count << '\n';
    std::cout << "Operations per thread: "
              << config.messages_per_thread << '\n';
    std::cout << "Total operations: " << total_operations << '\n';
    std::cout << "Repetitions: " << repetitions << '\n';
    std::cout << "========================================\n";

    for (int run = 1; run <= repetitions; ++run) {
        const BenchmarkResult result = run_once(config);

        if (result.accepted_messages != expected_accepted) {
            throw std::runtime_error(
                "Message verification failed in run " +
                std::to_string(run)
            );
        }

        throughput_results.push_back(
            result.operations_per_second
        );

        std::cout << "Run " << run
                  << " | Time: " << std::fixed
                  << std::setprecision(6)
                  << result.elapsed_seconds << " s"
                  << " | Throughput: " << std::setprecision(2)
                  << result.operations_per_second
                  << " operations/sec"
                  << " | Accepted: " << result.accepted_messages
                  << '\n';
    }

    std::sort(
        throughput_results.begin(),
        throughput_results.end()
    );

    const double median_throughput =
        throughput_results[repetitions / 2];

    std::cout << "\nMedian throughput: "
              << std::fixed << std::setprecision(2)
              << median_throughput
              << " operations/sec\n";

    std::cout << "Expected accepted messages: "
              << expected_accepted << '\n';

    std::cout << "Message verification: PASSED\n";
}

int main() {
    try {
        constexpr std::size_t messages_per_thread = 100000;

        std::cout << "C++ Logger Performance Benchmark\n";
        std::cout << "C++ Standard: C++17\n";
        std::cout << "Build configuration: Record as Release\n";

        const std::vector<BenchmarkConfig> benchmarks = {
            {
                "Single-thread enabled logging",
                1,
                messages_per_thread,
                LogLevel::TRACE,
                LogLevel::INFO
            },
            {
                "Two-thread enabled logging",
                2,
                messages_per_thread,
                LogLevel::TRACE,
                LogLevel::INFO
            },
            {
                "Four-thread enabled logging",
                4,
                messages_per_thread,
                LogLevel::TRACE,
                LogLevel::INFO
            },
            {
                "Eight-thread enabled logging",
                8,
                messages_per_thread,
                LogLevel::TRACE,
                LogLevel::INFO
            },
            {
                "Four-thread filtered logging",
                4,
                messages_per_thread,
                LogLevel::ERROR,
                LogLevel::INFO
            }
        };

        for (const auto& benchmark : benchmarks) {
            run_benchmark(benchmark);
        }

        std::cout << "\nAll benchmark scenarios completed.\n";
    }
    catch (const std::exception& exception) {
        std::cerr << "\nBenchmark failed: "
                  << exception.what() << '\n';

        return 1;
    }

    return 0;
}
