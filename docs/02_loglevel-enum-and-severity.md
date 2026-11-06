02 --- LogLevel: Scoped Enum, Underlying Type & Severity Mapping
1. WHY Do We Need LogLevel?
A Logger should not treat every message equally.

For example:
- TRACE can describe very detailed execution information.
- DEBUG can help during development and troubleshooting.
- INFO can report normal application events.
- WARN can report a potentially problematic situation.
- ERROR can report an operation that failed.
- FATAL can represent a severe failure.
A dedicated LogLevel type gives these categories clear names and
prevents the Logger API from relying on unexplained integer values or
arbitrary strings.
2. Current Implementation
File: include/logger/log_level.hpp
#pragma once

#include <cstdint>

namespace logging {

enum class LogLevel : std::uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
};

}
This is the reviewed implementation used in the Logger project.
3. Understanding the Declaration
3.1 enum
enum declares an enumeration: a type whose named values represent a
set of related choices.

Instead of writing unexplained values such as:
int level = 2;
we can write:
LogLevel level = LogLevel::INFO;
The second version communicates the meaning of the value.
3.2 enum class
enum class declares a scoped enumeration, also called a strongly typed
enum.

The values must be qualified with the enum name:
LogLevel::INFO
LogLevel::ERROR
This has useful properties:
- Enumerator names are scoped inside LogLevel.
- Names such as INFO do not get injected into the surrounding
  namespace.
- Values do not implicitly convert to ordinary integers.
- Accidental mixing with unrelated integer values is reduced.
Example:
LogLevel level = LogLevel::INFO;

// int value = level; // Not allowed implicitly.
An explicit conversion is possible when needed:
auto numericLevel = static_cast<std::uint8_t>(level);
Do not convert to an integer unless the numeric representation is
actually needed.
3.3 std::uint8_t
std::uint8_t is an unsigned integer type that is exactly 8 bits wide
when the implementation provides it.

An 8-bit unsigned value has 256 possible bit patterns, representing
values from 0 through 255.

Our Logger currently needs only six named levels, so this underlying
type is sufficient.

The underlying type specifies the integer representation used for the
enumeration. It does not mean that the enum has 256 named severity
levels.
3.4 Explicit numeric values
TRACE = 0,
DEBUG = 1,
INFO  = 2,
WARN  = 3,
ERROR = 4,
FATAL = 5
The values are explicitly assigned.

This makes the chosen numeric mapping visible and stable in the source.

The ordering is useful for severity comparisons, provided the project
consistently treats larger values as more severe.

For example:
if (level >= LogLevel::ERROR) {
    // Handle ERROR or FATAL severity.
}
Because enum class does not implicitly support relational comparison
with integers, compare enum values with enum values:
if (level >= LogLevel::ERROR) {
    // ...
}
4. Severity Levels
  Level       Numeric value Intended meaning
  TRACE                 0 Very detailed execution information
DEBUG                 1 Diagnostic information useful during development
INFO                  2 Normal application events
WARN                  3 Something unexpected or potentially problematic
ERROR                 4 An operation or task failed
FATAL                 5 A severe failure that may prevent continued operation

These descriptions are the project's intended severity meanings. A later
Logger filtering policy will define exactly which levels are emitted for
a configured threshold.
5. Why Not Use Plain enum?
A traditional unscoped enum can allow enumerator names to leak into the
surrounding scope and can convert to integers implicitly.

For a reusable library, enum class is generally safer because it makes
the type boundary explicit.

Compare:
enum OldLevel {
    INFO,
    ERROR
};
with:
enum class LogLevel : std::uint8_t {
    INFO,
    ERROR
};
With the scoped enum, use:
LogLevel::INFO
This also reduces naming collisions with other parts of a program.
6. Using LogLevel in a Switch
The LogFormatter::toString() function maps each enum value to readable
text.
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
Why use switch?
- Each supported level is handled explicitly.
- The mapping is easy to read.
- A default branch gives a fallback for an unrecognized enum value.
Why does each case use return instead of break?
return exits the entire function and supplies its result.

If every case returns, a break is not needed.

By contrast, break exits only the switch statement and execution
continues after it.
Why keep default?
Although the six named enumerators are the expected values, an enum
object can still contain an unnamed value---for example, if a program
explicitly converts an integer to LogLevel.

The default branch makes the formatter's behavior for such a value
explicit:
default:
    return "UNKNOWN";
This fallback does not make arbitrary enum values valid severity levels.
It only handles an unexpected value safely at this formatting boundary.
7. Where LogLevel Is Used in the Logger
LogLevel is used by LogRecord:
struct LogRecord {
    std::string message;
    LogLevel level;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id thread_id;
};
The level member records the severity associated with a particular log
event.

A LogRecord can then be formatted using:
LogFormatter::toString(record.level);
The future Logger component will also use severity levels when
applying its configured filtering policy.
8. Important Concepts and Common Mistakes
Concept: Type versus value
LogLevel level = LogLevel::INFO;
- LogLevel is the type.
- level is the variable.
- LogLevel::INFO is the enum value assigned to that variable.
Common mistake: Forgetting the scope
Incorrect:
LogLevel level = INFO;
Correct:
LogLevel level = LogLevel::INFO;
Common mistake: Treating the enum as an ordinary integer
Incorrect:
int level = LogLevel::INFO;
Correct when a numeric representation is genuinely required:
auto levelNumber =
    static_cast<std::uint8_t>(LogLevel::INFO);
Common mistake: Assuming the underlying type creates the named values
std::uint8_t determines the underlying representation. The six named
severity values are the enumerators explicitly declared in LogLevel.
9. Quick Reference
  Syntax                               Meaning
  enum class                         Declares a scoped, strongly typed
enumeration

  LogLevel                           Enumeration type

  LogLevel::INFO                     A named enumerator

  : std::uint8_t                     Specifies the underlying integer
type

  static_cast<std::uint8_t>(level)   Explicitly converts the enum value
to its underlying numeric
representation

  switch (level)                     Selects a branch based on the enum
value
default                            Handles values not matched by the
listed cases
10. Progress
- [x] LogLevel enum declared.
- [x] Six severity values assigned explicit numeric values.
- [x] std::uint8_t selected as the underlying type.
- [x] enum class used for scoped, strongly typed values.
- [x] LogFormatter::toString() implemented with all six cases and a
  fallback.
Next related topic: LogRecord --- how the Logger stores a message,
severity, timestamp and thread ID together.