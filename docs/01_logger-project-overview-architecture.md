01 --- Logger Project Overview, Architecture & Design Decisions
1. Project Overview
WHY are we building a Logger?
In a real software system, std::cout is not enough for professional
logging.

A reusable Logger should help an application:
- Record what happened during execution.
- Identify the severity of an event.
- Capture when the event happened.
- Identify which thread produced the event.
- Send log messages to different destinations.
- Keep logging logic separate from application logic.
The goal of this project is to build a modular, reusable C++ Logger
rather than scattering std::cout and file-writing code throughout an
application.
Project identity
  Item                          Decision
  Project number                09
Project name                  Logger
Language standard             C++17
Initial library type          Static library
Initial output destinations   Console and file
Execution model               Synchronous
Thread-safety approach        std::mutex with RAII locking
Logger access                 Dependency Injection (DI), not a global singleton
Future extension              Asynchronous logging can be considered later

These are the project's agreed initial design decisions. They are not
claims that every feature has already been implemented.
2. Requirements
Functional requirements
The Logger is intended to:
1. Support six severity levels: TRACE, DEBUG, INFO, WARN, ERROR and
   FATAL.
2. Capture a message, severity, timestamp and thread ID in a log
   record.
3. Convert a log record into a readable string.
4. Write logs to the console.
5. Write logs to a file.
6. Allow the Logger to use different output destinations through a
   common interface.
Engineering requirements
- Keep components separated by responsibility.
- Avoid coupling the Logger directly to a particular output
  destination.
- Use clear public interfaces and implementation files.
- Protect shared logging operations with a mutex.
- Use RAII for lock management.
- Verify behavior with tests rather than treating a successful build
  as proof of correctness.
- Keep documentation and test evidence aligned with the actual
  implementation.
3. High-Level Architecture
The intended flow is:
Application component
        |
        | Logger reference supplied through Dependency Injection
        v
      Logger
        |
        | Create LogRecord
        | Apply severity filtering
        | Coordinate synchronization
        v
   LogFormatter
        |
        | Convert LogRecord into formatted text
        v
     LogSink interface
        |
        +--------------------+
        |                    |
        v                    v
   ConsoleSink            FileSink
        |                    |
        v                    v
     Console             Log file
Responsibility of each component
  Component                           Responsibility
  LogLevel                          Defines the supported severity
levels.

  LogRecord                         Carries the information associated
with one log event.

  LogFormatter                      Converts log data into readable
text.

  LogSink                           Defines a common output interface.

  ConsoleSink                       Writes formatted messages to the
console.

  FileSink                          Writes formatted messages to a
file.
Logger                            Provides the application-facing API
and coordinates logging.
The diagram describes the intended architecture. Components should only
be marked complete after their implementation has been reviewed and
tested.
4. Why Separate Components?
Imagine putting all logging behavior inside one large Logger class:
- Severity conversion
- Timestamp conversion
- Console output
- File output
- Thread synchronization
- File management
This would make the class harder to understand, test and extend.

Instead, each component has a focused responsibility.
Separation of concerns
- LogRecord stores data; it does not format or print it.
- LogFormatter formats data; it does not decide where the output
  goes.
- LogSink defines the output contract.
- Concrete sinks perform destination-specific output.
- Logger coordinates the overall logging operation.
This separation makes individual components easier to reason about and
test.
5. Important Design Decisions
5.1 Synchronous logging initially
The initial Logger will perform logging synchronously.

This means the calling thread performs the logging operation rather than
placing the log record into a queue for a background worker.

Why start synchronously?
- The control flow is easier to understand.
- There is no worker-thread lifecycle to manage initially.
- There is no queue shutdown or queue-draining behavior to implement
  initially.
- It provides a simpler foundation before considering asynchronous
  logging.
Asynchronous logging is a possible future extension, not part of the
initial scope.
5.2 Thread safety with std::mutex
Multiple threads may attempt to log at the same time.

If they write to shared output without suitable synchronization, their
operations may interfere with each other. Concurrent unsynchronized
access to shared non-atomic state can cause a data race and undefined
behavior.

The initial design uses:
std::mutex mutex;
A mutex provides mutual exclusion when code consistently locks it around
the relevant shared operation.

Important: Merely declaring a mutex does not protect anything. The
relevant code must actually acquire the mutex.
5.3 RAII with std::lock_guard
The preferred locking style is:
std::lock_guard<std::mutex> lock(mutex);
std::lock_guard acquires the mutex when constructed and releases it
when destroyed.

This is RAII: resource acquisition and release are tied to object
lifetime.

It also helps ensure that the mutex is released when execution leaves
the scope, including during exception unwinding.
5.4 Dependency Injection instead of a global singleton
Application components should receive a Logger reference or suitable
dependency rather than relying on a global Logger singleton.

Conceptually:
void runApplication(Logger& logger);
The caller supplies the Logger dependency.

Benefits include:
- Dependencies are visible in function interfaces.
- Tests can supply a test-specific Logger or sink configuration.
- Different application components can use explicitly supplied logging
  dependencies.
- The design avoids hidden global access.
This is the selected project approach.
6. Current Project Structure
The project structure established so far is:
09-logger/
|
|-- CMakeLists.txt
|-- README.md
|-- .gitignore
|
|-- include/
|   `-- logger/
|       |-- logger.hpp
|       |-- log_level.hpp
|       |-- log_record.hpp
|       |-- log_formatter.hpp
|       |-- log_sink.hpp
|       `-- console_sink.hpp
|
|-- src/
|   |-- logger.cpp
|   |-- log_formatter.cpp
|   `-- console_sink.cpp
|
|-- tests/
|-- examples/
|-- benchmarks/
`-- docs/
This is the intended working layout based on the project structure
discussed so far. Some files may still be skeletons or have pending
implementation.
7. Build System
The project uses CMake and a static library target.

The source files must be listed in add_library() so CMake compiles
them into the library.

Example:
add_library(logger STATIC
    src/logger.cpp
    src/log_formatter.cpp
)
As additional .cpp implementation files are added, they must also be
added to the target.
Existing build commands
The established Windows/MinGW build workflow is:
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
A successful build confirms that the configured source files compile and
the target links. It does not prove that all runtime behavior is
correct. Tests are required for that.
8. Development Environment
The working environment established for this project:
- Windows
- VS Code
- MSYS2 UCRT64
- GCC / g++ 16.2.0
- CMake
- MinGW Makefiles generator
An existing WSL Ubuntu environment is also available for Linux-oriented
work. Do not reinstall or reconfigure a working toolchain without a
specific reason.
9. Engineering Workflow
For each component, follow this workflow:
1. WHY --- Understand the problem the component solves.
2. Requirements --- Identify what it must and must not do.
3. Design --- Decide its responsibility and public interface.
4. Implementation --- Write a small part at a time.
5. Code review --- Check correctness, naming, ownership, lifetime
   and edge cases.
6. Build --- Compile the project.
7. Testing --- Verify behavior with meaningful test cases.
8. Documentation --- Record design, behavior and limitations.
9. Evidence --- Preserve genuine build/test/benchmark results where
   relevant.
A successful compile is not the same as a successful test.

Do not invent test results, benchmark numbers or performance claims.
10. Progress Snapshot
At the point these notes were created, the following work had been
discussed and implemented to varying degrees:
  Area                                Progress
  Project requirements and initial    Discussed and agreed
architecture                        

  LogLevel enum                     Implemented and reviewed

  LogRecord structure               Implemented and reviewed

  LogFormatter declarations         Created

  LogFormatter::toString()          Implemented

  LogFormatter::formatTimestamp()   Implemented; error handling still
needs review

  LogFormatter::format()            Implemented

  CMake integration for formatter     Build succeeded

  LogSink interface                 Discussed; header created or being
set up

  ConsoleSink                       Next implementation area

  FileSink                          Pending

  Main Logger implementation        Pending

  Unit tests                          Pending
Final documentation and audit       Pending
Progress note: A successful build was observed after adding
src/log_formatter.cpp to the CMake library target. This verifies
compilation/linking for that build, not the correctness of runtime
formatting behavior.
11. Quick Reference
  Term                                Meaning in this project
  Logger                              Application-facing component that
coordinates logging.

  Log record                          Data describing one logging event.

  Formatter                           Converts structured log data into
readable text.

  Sink                                Destination abstraction for log
output.

  Synchronous                         The calling thread performs the
logging work.

  Mutex                               Synchronization primitive providing
mutual exclusion when locked.

  RAII                                Resource lifetime is managed by
object lifetime.

  Dependency Injection                A dependency is supplied by the
caller instead of obtained through
hidden global access.
Static library                      Compiled library code packaged for
linking into another program.
12. Next Topics
Continue with separate notes for:
- LogLevel: scoped enums and underlying types.
- LogRecord: structs, aggregate initialization and timestamp/thread
  fields.
- LogFormatter: function declarations/definitions, time conversion
  and string formatting.
- OOP foundations used by sinks: abstract classes, pure virtual
  functions, virtual destructors, inheritance, overriding and runtime
  polymorphism.
- LogSink, ConsoleSink and FileSink.
- Main Logger implementation, synchronization, testing and final
  audit.