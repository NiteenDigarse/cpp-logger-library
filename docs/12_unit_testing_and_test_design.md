# 12. Unit Testing and Test Design

## 1. WHY — Why Do We Need Unit Testing?

Writing code is only one part of software development.

A component may compile successfully but still produce incorrect results during execution.

For example:

- A LogLevel conversion function may return an incorrect string.
- A LogFormatter may generate an incorrectly formatted timestamp.
- A FileSink may overwrite existing log messages.
- A Logger may accept messages that should have been filtered.
- A Logger may fail when accessed by multiple threads.

Compilation verifies that the code satisfies the compiler's requirements. It does not prove that the implementation behaves correctly.

Unit testing helps verify the behavior of individual components.

It allows developers to detect errors early, validate expected behavior, and reduce the risk of breaking existing functionality when code changes.

## 2. Concept — What Is Unit Testing?

Unit testing is a software testing technique in which individual units of a program are tested independently.

A unit is a small, logically identifiable part of a program.

Examples from our Logger project:

| Component | Unit Under Test |
|---|---|
| LogLevel | Level-to-string conversion |
| LogFormatter | Timestamp and message formatting |
| ConsoleSink | Terminal output |
| FileSink | File writing and error handling |
| Logger | Filtering, sink distribution, and synchronization |

The purpose is to verify that each component behaves according to its expected responsibilities.

## 3. Mental Model — Unit Testing Flow

```text
       Test Case
           |
           v
    Prepare Input
           |
           v
    Execute Function
           |
           v
    Capture Output
           |
           v
 Compare Expected Result
           |
       +---+---+
       |       |
       v       v
      PASS    FAIL
```

A test passes when the actual result satisfies the expected condition.

A test fails when the expected condition is not satisfied.

## 4. Test Structure

A common unit testing pattern is AAA:

1. Arrange
2. Act
3. Assert

### 4.1 Arrange

Prepare the objects, inputs, and initial state required by the test.

### 4.2 Act

Execute the function or operation being tested.

### 4.3 Assert

Verify that the actual result matches the expected behavior.

Example:

```cpp
void testLogLevel() {

    // Arrange
    logging::LogLevel level = logging::LogLevel::INFO;

    // Act
    std::string result = logging::LogFormatter::toString(level);

    // Assert
    if (result == "INFO") {
        std::cout << "PASS\n";
    } else {
        std::cout << "FAIL\n";
    }
}
```

AAA makes test cases easier to read and maintain.

## 5. Test Organization in Our Logger Project

Our project separates tests from production source code.

```text
09-logger/
|
+-- include/
|   +-- logger/
|
+-- src/
|   +-- logger.cpp
|   +-- log_formatter.cpp
|   +-- console_sink.cpp
|   +-- file_sink.cpp
|
+-- tests/
    +-- test_log_level.cpp
    +-- test_log_formatter.cpp
    +-- test_console_sink.cpp
    +-- test_file_sink.cpp
    +-- test_logger.cpp
```

This separation provides several benefits:

- Production code remains independent of test implementation.
- Tests can be modified without changing library code.
- Each component has a dedicated test file.
- Test coverage can be expanded independently.

## 6. LogLevel Unit Testing

File: `tests/test_log_level.cpp`

The LogLevel tests verify the string representation of every defined severity.

Expected mappings:

| Input | Expected Result |
|---|---|
| TRACE | TRACE |
| DEBUG | DEBUG |
| INFO | INFO |
| WARN | WARN |
| ERROR | ERROR |
| FATAL | FATAL |

The tests also check an invalid enum value.

Example:

```cpp
auto invalid_level =
    static_cast<logging::LogLevel>(255);
```

Expected result:

```text
UNKNOWN
```

This verifies that the conversion function handles an unsupported value through its default case.

## 7. LogFormatter Unit Testing

File: `tests/test_log_formatter.cpp`

LogFormatter is responsible for converting a structured LogRecord into a formatted string.

Our tests verify:

- Severity text appears in the output.
- The original message is present.
- Thread information is included.
- Timestamp formatting follows the expected structure.

Example expected format:

```text
[2026-11-06 11:29:58] [INFO] [Thread: 1] Application started
```

A timestamp format check can verify its length and separators.

Expected timestamp structure:

```text
YYYY-MM-DD HH:MM:SS
```

The test also uses an epoch-based timestamp to verify predictable formatting.

## 8. ConsoleSink Unit Testing

File: `tests/test_console_sink.cpp`

ConsoleSink writes messages to standard output.

To test it without relying on manually observing the terminal, we can redirect `std::cout` to an `std::ostringstream`.

Example:

```cpp
std::ostringstream captured;

std::streambuf* original =
    std::cout.rdbuf(captured.rdbuf());

sink.write("Application started");

std::cout.rdbuf(original);
```

Now the captured output can be inspected programmatically.

The tests cover:

- Basic message output.
- Multiple messages.
- Empty messages.
- Special characters.
- Newline behavior.
- Flush execution.

This is an example of controlling an output dependency to make a component easier to test.

## 9. FileSink Unit Testing

File: `tests/test_file_sink.cpp`

FileSink interacts with the file system.

Its tests verify important file operations.

### Test scenarios

1. File creation and writing.
2. Append behavior.
3. Multiple message writing.
4. Empty message handling.
5. Special character handling.
6. File-opening failure.

Example:

```cpp
logging::FileSink sink("test.log");

sink.write("Application started");
sink.flush();
```

The test can open the resulting file and inspect its contents.

For append testing, the test writes additional content and verifies that previous content remains present.

For failure testing, it uses a path that cannot be opened under the test environment.

**Important:** File-system tests depend on environment permissions and paths. A reliable test should use controlled test paths and clean up temporary files where appropriate.

## 10. Logger Unit Testing

File: `tests/test_logger.cpp`

Logger is the central component, so it requires tests covering interactions between multiple parts of the logging system.

Our Logger test suite contains 12 test scenarios.

| No. | Test Scenario | Expected Behavior |
|---:|---|---|
| 1 | Valid constructor | Logger is created successfully |
| 2 | Empty sink vector | Throws invalid_argument |
| 3 | Null sink | Throws invalid_argument |
| 4 | All logging methods | All six levels are delivered |
| 5 | Minimum-level filtering | Lower levels are rejected |
| 6 | Runtime level change | Updated threshold is applied |
| 7 | getLevel() | Correct configured level is returned |
| 8 | Multiple sinks | All configured sinks receive messages |
| 9 | flush() | Every sink receives a flush request |
| 10 | Empty message | Empty message is accepted |
| 11 | Multithreaded logging | 400 messages are received |
| 12 | Message integrity | All received messages contain expected information |

These tests verify both individual Logger operations and interactions with injected sinks.

## 11. Positive and Negative Testing

### 11.1 Positive Testing

Positive tests verify that valid inputs and normal operations work correctly.

Examples:

- Constructing a Logger with valid sinks.
- Logging an INFO message.
- Writing to a valid file.
- Formatting a valid LogRecord.

### 11.2 Negative Testing

Negative tests verify that the system handles invalid conditions correctly.

Examples:

- Constructing a Logger without sinks.
- Supplying a null sink.
- Opening a file in a nonexistent directory.

Expected behavior should be clearly defined for every negative test.

For example:

```cpp
try {
    logging::Logger logger({});
}
catch (const std::invalid_argument&) {
    // Expected exception
}
```

Negative testing is important because production software must handle failure conditions, not just successful execution.

## 12. Testing Exceptions

Exceptions are part of a component's observable behavior.

When a function is expected to reject invalid input, the test should verify that the correct exception is thrown.

Example:

```cpp
bool exception_thrown = false;

try {
    logging::Logger logger({});
}
catch (const std::invalid_argument&) {
    exception_thrown = true;
}

if (exception_thrown) {
    std::cout << "PASS\n";
} else {
    std::cout << "FAIL\n";
}
```

A stronger test should also fail if an unexpected exception type is thrown.

This ensures that the implementation follows its intended error-handling contract.

## 13. Custom Test Harness

Our test files use a simple test harness to execute test scenarios and report results.

A test harness generally needs to:

- Execute individual test cases.
- Record successful tests.
- Record failed tests.
- Display useful failure information.
- Return a non-zero exit code when tests fail.

For example:

```cpp
int passed = 0;
int failed = 0;
```

The counters track the test results.

The program should return:

```cpp
return failed == 0 ? 0 : 1;
```

A zero exit code indicates success.

A non-zero exit code indicates that at least one test failed.

This is important because CTest and automated build systems use process exit codes to determine whether a test passed.

## 14. Unit Testing vs Integration Testing

These testing techniques have different purposes.

### Unit Testing

Tests a component or a small unit in isolation.

Examples:

- Testing LogFormatter independently.
- Testing FileSink independently.
- Testing Logger with a custom TestSink.

### Integration Testing

Tests whether multiple components work together correctly.

Our Logger example integrates:

- Logger.
- LogFormatter.
- ConsoleSink.
- FileSink.

It verifies that a logging request travels through the complete logging pipeline.

| Testing Type | Main Focus |
|---|---|
| Unit Testing | Individual component behavior |
| Integration Testing | Interaction between components |
| System Testing | Complete application behavior |

A project can use all three levels as it grows.

## 15. Running Tests Through CTest

Our project uses CMake to build test executables and CTest to execute registered tests.

The relevant CMake configuration is:

```cmake
enable_testing()

add_test(NAME LogLevelTest
    COMMAND test_log_level
)

add_test(NAME LogFormatterTest
    COMMAND test_log_formatter
)

add_test(NAME ConsoleSinkTest
    COMMAND test_console_sink
)

add_test(NAME FileSinkTest
    COMMAND test_file_sink
)

add_test(NAME LoggerTest
    COMMAND test_logger
)
```

Each `add_test()` command registers a test with CTest.

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

The `--output-on-failure` option displays additional output for tests that fail.

## 16. Actual Test Results

After integrating the Logger test executable, we successfully built and executed all five registered test suites.

Observed result:

```text
Test project ...

1/5 Test #1: LogLevelTest ............ Passed
2/5 Test #2: LogFormatterTest ........ Passed
3/5 Test #3: ConsoleSinkTest ......... Passed
4/5 Test #4: FileSinkTest ............ Passed
5/5 Test #5: LoggerTest .............. Passed

100% tests passed, 0 tests failed out of 5
```

Total registered test suites: 5

Passed test suites: 5

Failed test suites: 0

This confirms that all five registered test executables passed in the recorded test run.

**Important:** Five passing test suites do not mean that every possible behavior or edge case has been tested. Test coverage must be evaluated separately.

## 17. What Makes a Good Unit Test?

A useful test should have the following properties:

### Independent

One test should not depend on the execution order or result of another test.

### Repeatable

Running the same test under the same conditions should produce consistent results.

### Focused

Each test should verify a clearly defined behavior.

### Deterministic

Tests should avoid relying on unpredictable timing or external state whenever possible.

### Readable

The purpose and expected result should be clear.

### Useful Failure Messages

When a test fails, its output should help identify the problem.

## 18. Current Testing Limitations

Our current testing setup is a useful foundation, but it is not exhaustive.

Possible future improvements include:

- More detailed assertion helpers.
- Better temporary-file management.
- Explicit sink exception tests.
- Invalid LogLevel configuration tests.
- Tests for concurrent setLevel() and logging.
- Sanitizer-based checks.
- More detailed integration tests.
- Code coverage measurement.

These should be added based on actual requirements and engineering risks.

## 19. Important Engineering Points

- Successful compilation does not prove correct runtime behavior.
- Unit testing verifies individual component behavior.
- AAA means Arrange, Act, Assert.
- Positive tests verify valid operations.
- Negative tests verify failure handling.
- Exception behavior should be tested explicitly.
- Test executables should return meaningful exit codes.
- CTest executes registered test programs.
- Integration testing verifies interactions between components.
- Passing tests do not prove complete correctness.
- Test coverage and test quality are different measurements.

## 20. Quick Reference

| Term | Meaning |
|---|---|
| Unit Test | Test of an individual unit |
| Test Case | A specific behavior being verified |
| Arrange | Prepare test conditions |
| Act | Execute the operation |
| Assert | Verify expected behavior |
| Positive Test | Verify valid input and operation |
| Negative Test | Verify invalid input and failure handling |
| Test Harness | Executes and reports test cases |
| Integration Test | Verifies component interactions |
| CTest | CMake's test execution tool |
| Exit Code 0 | Successful process execution |
| Non-zero Exit Code | Failed process execution |

## 21. Progress

**Completed:**

- Separate test files for Logger components.
- LogLevel unit tests.
- LogFormatter unit tests.
- ConsoleSink unit tests.
- FileSink unit tests.
- Logger unit tests.
- Positive and negative test scenarios.
- Exception testing.
- Multithreaded Logger tests.
- CMake test registration.
- CTest execution.
- Successful 5/5 test-suite run.

**Next Topic:**

`13-test-doubles-and-concurrent-testing.md`

We will study how our custom TestSink replaces real output destinations during testing, why test doubles are useful, and how concurrent Logger behavior is verified.