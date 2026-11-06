04 --- LogFormatter: Function Signatures, Chrono Conversion & String Formatting
1. WHY Do We Need LogFormatter?
A LogRecord stores structured event data:
- Message
- Severity level
- Timestamp
- Thread ID
That data is useful inside the program, but logs should be readable by a
developer or operator.

LogFormatter has one focused responsibility: convert a LogRecord
into a readable std::string.

It does not decide whether a message should be logged, and it does not
decide where the message should be written. Those responsibilities
belong to other Logger components.
2. Header Declaration
File: include/logger/log_formatter.hpp

The reviewed declaration is:
#pragma once

#include <string>
#include <chrono>

#include "logger/log_level.hpp"
#include "logger/log_record.hpp"

namespace logging {

class LogFormatter {
public:
    static std::string toString(LogLevel level);

    static std::string formatTimestamp(
        std::chrono::system_clock::time_point timestamp
    );

    static std::string format(const LogRecord& record);
};

}
Why include these headers?
- <string> is needed because the public functions return
  std::string.
- <chrono> is needed because the public interface directly names
  std::chrono::system_clock::time_point.
- log_level.hpp is needed because toString() directly uses
  LogLevel.
- log_record.hpp is needed because format() directly uses
  LogRecord.
A header should include the declarations for the types it directly uses.
Do not rely unnecessarily on another header to include a dependency
indirectly.
Why was <string> duplicated before?
The header temporarily contained two <string> includes. Only one is
needed. Repeated inclusion of standard headers is generally protected by
include guards, but duplicate includes are unnecessary clutter.
3. Understanding the Function Signature
Consider:
std::string LogFormatter::formatTimestamp(
    std::chrono::system_clock::time_point timestamp
);
Break it into parts:

  Part                                      Meaning
  std::string                             Function return type
LogFormatter                            Class that owns the member function
::                                      Scope resolution operator
formatTimestamp                         Function name
std::chrono::system_clock::time_point   Parameter type
timestamp                               Parameter name
Important distinction
LogFormatter is not the return type and it is not the source filename.
It is the class name that scopes the function definition.

The function returns a std::string.

The parameter type is std::chrono::system_clock::time_point, and the
parameter variable is named timestamp.
Calling the function
Conceptually:
auto now = std::chrono::system_clock::now();

std::string text = LogFormatter::formatTimestamp(now);
The value in now is passed into the function's timestamp parameter.
4. Why Are the Functions Static?
The declarations use static:
static std::string toString(LogLevel level);
static std::string formatTimestamp(
    std::chrono::system_clock::time_point timestamp
);
static std::string format(const LogRecord& record);
These are static member functions.

A static member function can be called using the class name without
creating a LogFormatter object:
auto levelText = LogFormatter::toString(LogLevel::INFO);
Static does not mean that the function is automatically thread-safe.
Thread safety depends on what the function accesses and how that data is
managed.

These formatter functions are designed as stateless utility operations:
their inputs are supplied as parameters, and they produce strings.
5. toString(LogLevel level)
Responsibility
Convert a severity enum value into readable text.

Example:
std::string LogFormatter::toString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::TRACE:
            return "TRACE";

        case LogLevel::DEBUG:
            return "DEBUG";

        case LogLevel::INFO:
            return "INFO";

        case LogLevel::WARN:
            return "WARN";

        case LogLevel::ERROR:
            return "ERROR";

        case LogLevel::FATAL:
            return "FATAL";

        default:
            return "UNKNOWN";
    }
}
Why use a switch?
Each named enum value is handled explicitly. The default branch
provides a fallback if an unexpected enum value is supplied.
return versus break
- return exits the entire function and returns a result.
- break exits the switch statement only.
Because each case returns, a break is not needed.
6. formatTimestamp() --- The Conversion Pipeline
Responsibility
Convert a std::chrono::system_clock::time_point into a readable local
date/time string.

The intended output pattern is:
2026-11-05 20:07:00
The implementation discussed in this project follows this pipeline:
system_clock::time_point
          |
          v
system_clock::to_time_t()
          |
          v
       std::time_t
          |
          v
 localtime_s() / localtime_r()
          |
          v
       std::tm
          |
          v
 std::put_time() + ostringstream
          |
          v
      std::string
Step 1: Convert time_point to time_t
std::time_t time =
    std::chrono::system_clock::to_time_t(timestamp);
- timestamp is the input time point.
- to_time_t() converts it to the traditional calendar-time
  representation.
- std::time_t is commonly represented as an arithmetic count of time
  units, typically seconds, from an implementation-defined epoch
  conventionally associated with 1 January 1970.
Do not assume that time_t is a formatted date string.
Step 2: Create a std::tm object
std::tm tm{};
std::tm is a calendar-time structure with fields such as year, month,
day, hour, minute and second.

The {} value-initializes the structure. It does not automatically
convert the time_t value into calendar fields.

A conversion function must populate it.
Step 3: Convert to local calendar time
The project implementation uses platform-specific functions:
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
- On the Windows branch, localtime_s() is used.
- On the POSIX branch, localtime_r() is used.
- Both write calendar fields into the caller-provided tm object.
The argument order differs:

  Function                Argument order
  Windows localtime_s   &tm, &time
POSIX localtime_r     &time, &tm

These functions are preferred over relying on std::localtime() in
concurrent code because they write into caller-provided storage rather
than returning a pointer to shared static calendar storage.

Portability note: The exact availability and declaration of the
Windows localtime_s function can depend on the compiler and C runtime.
Verify the chosen branch with the actual MinGW toolchain. Also check the
return status in production-quality code; the current learning
implementation does not yet handle conversion failure.
Step 4: Create a string stream
std::ostringstream oss;
std::ostringstream builds text in memory.

It is appropriate because this function must return a string, not print
directly to the console.

Compare:
  Facility                            Purpose
  std::cout                         Writes output to standard console
output
std::ostringstream                Builds an in-memory string that can
be retrieved
Example:
std::ostringstream oss;
oss << "Hello " << "Logger";

std::string result = oss.str();
The result string contains Hello Logger.
Step 5: Format the calendar fields
oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
std::put_time() formats a std::tm structure according to a format
string.

  Specifier   Meaning                                               Example
  %Y        Four-digit year                                       2026
%m        Month, 01--12                                         11
%d        Day of month                                          05
%H        Hour, 00--23                                          20
%M        Minute, 00--59                                        07
%S        Second, 00--60 depending on calendar representation   00

The separators (-, space and :) are literal characters in the format
string.
Step 6: Return the string
return oss.str();
str() retrieves the accumulated string from the string stream.

The function returns that std::string to its caller.
7. Current formatTimestamp() Implementation
The implementation discussed so far is:
std::string LogFormatter::formatTimestamp(
    std::chrono::system_clock::time_point timestamp)
{
    // Step 1: Convert time_point into time_t
    std::time_t time =
        std::chrono::system_clock::to_time_t(timestamp);

    // Step 2: Create tm structure
    std::tm tm{};

    // Step 3: Convert time_t into local calendar time
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    // Step 4: Create string stream
    std::ostringstream oss;

    // Step 5: Format date and time
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // Step 6: Return formatted string
    return oss.str();
}
Required headers used by this implementation:
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
The public header should also directly include <chrono> because it
names a chrono type in its function declaration.
8. format(const LogRecord& record)
Responsibility
Combine the four fields of a LogRecord into one formatted string.

The declaration is:
static std::string format(const LogRecord& record);
Breakdown:
- std::string --- return type.
- format --- function name.
- const LogRecord& --- read-only reference parameter.
- record --- parameter name.
Why const LogRecord&?
- & avoids making a copy of the entire record.
- const prevents the formatter from modifying the record through
  this reference.
Implementation
std::string LogFormatter::format(const LogRecord& record)
{
    std::ostringstream oss;

    // Timestamp
    oss << "["
        << LogFormatter::formatTimestamp(record.timestamp)
        << "] ";

    // Log level
    oss << "["
        << LogFormatter::toString(record.level)
        << "] ";

    // Thread ID
    oss << "[Thread: "
        << record.thread_id
        << "] ";

    // Message
    oss << record.message;

    return oss.str();
}
How the fields are accessed
  Expression           Meaning
  record.timestamp   Access the record's timestamp member
record.level       Access its severity
record.thread_id   Access its thread ID
record.message     Access its message

The formatter reuses the other formatter functions:
- formatTimestamp() converts the timestamp.
- toString() converts the severity.
Example output shape
[2026-11-05 20:07:00] [INFO] [Thread: <thread-id>] Application started
The exact textual representation of std::thread::id is
implementation-dependent. The example shows the output shape, not a
guaranteed numeric thread ID.
9. Complete log_formatter.cpp Shape
The source file contains three function definitions inside the logging
namespace:
log_formatter.cpp
|
|-- toString(LogLevel)
|     `-- switch over the severity values
|
|-- formatTimestamp(time_point)
|     `-- time_point -> time_t -> tm -> formatted string
|
`-- format(LogRecord)
      `-- timestamp + level + thread ID + message
A function's declaration belongs in the header; its definition belongs
in the source file for this project.

The LogFormatter class is declared in log_formatter.hpp. The .cpp
file provides the definitions of its static member functions.
10. Important Limitations and Review Items
The current implementation is a useful first version, but it is not yet
a fully audited production formatter.

Review these points before calling it production-ready:
1. Check conversion errors: Check the return status of
   localtime_s() or localtime_r() and define what happens if
   conversion fails.
2. Verify toolchain portability: Compile the Windows branch with
   the actual MinGW/UCRT toolchain and the POSIX branch on Linux.
3. Time-zone behavior: The current use of local-time conversion
   formats according to the machine's local time zone. It does not
   produce an explicitly UTC timestamp.
4. Sub-second precision: The current format includes whole seconds
   only. It does not include milliseconds or microseconds.
5. Thread ID representation: The printed thread ID is
   implementation-dependent.
6. Formatting failure behavior: Decide how the formatter should
   report a failed time conversion.
7. Tests: Test every severity mapping, unexpected enum values,
   timestamp formatting and complete record formatting.
Do not claim these cases have been tested until actual tests have been
written and run.
11. Quick Reference
  Concept                             Meaning
  std::string                       String return type

  LogFormatter::                    Function belongs to the
LogFormatter class

  ::                                Scope resolution operator

  static member function            Can be called using the class name
without a formatter object

  time_point                        Specific point on a clock's
timeline

  system_clock::now()               Captures current system-clock time

  system_clock::to_time_t()         Converts a system-clock time point
to time_t

  std::time_t                       Traditional calendar-time
representation

  std::tm                           Calendar fields structure

  localtime_s()                     Windows local calendar-time
conversion function

  localtime_r()                     POSIX reentrant local calendar-time
conversion function

  std::ostringstream                Builds a string in memory

  std::put_time()                   Formats a std::tm value

  .str()                            Retrieves the string from a string
stream
const LogRecord&                  Read-only reference; avoids copying
the record
12. Progress
- [x] LogFormatter class declarations created.
- [x] Direct header dependencies reviewed.
- [x] toString() implemented.
- [x] formatTimestamp() implemented.
- [x] format() implemented.
- [x] src/log_formatter.cpp added to the CMake library target.
- [x] Build completed successfully after formatter integration.
- [ ] Runtime unit tests for formatter behavior.
- [ ] Error handling and cross-platform behavior audit.
Observed evidence: The build command reported
[100%] Built target logger after src/log_formatter.cpp was added to
the library target. This establishes a successful build for that
configuration; it does not establish that all formatter behaviors have
passed runtime tests.

Next related topic: OOP concepts used by LogSink: abstract
classes, pure virtual functions, virtual destructors, inheritance,
overriding and runtime polymorphism.