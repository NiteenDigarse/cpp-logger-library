03 --- LogRecord: Struct, Aggregate Initialization, Timestamp & Thread ID
1. WHY Do We Need LogRecord?
A log event contains more than just a message.

For example, this message:
Application started
does not tell us:
- When did it happen?
- Which severity was assigned?
- Which thread produced it?
A LogRecord groups the information about one logging event into a
single object.

This keeps event data structured instead of passing several unrelated
values around.
2. Current Implementation
File: include/logger/log_record.hpp
#pragma once

#include <string>
#include <chrono>
#include <thread>

#include "logger/log_level.hpp"

namespace logging {

struct LogRecord {
    std::string message;
    LogLevel level;

    std::chrono::system_clock::time_point timestamp;

    std::thread::id thread_id;
};

}
The structure contains four data members:
  Member                  Type                                      Purpose
  message               std::string                             Text describing the
event

  level                 LogLevel                                Severity of the event

  timestamp             std::chrono::system_clock::time_point   Point in time when the
event was recorded
thread_id             std::thread::id                         ID of the thread that
recorded the event
LogRecord is a data-carrying type. It does not print, format, filter
or write logs.
3. Why Use a struct?
In C++, both struct and class can contain:
- Data members
- Member functions
- Constructors and destructors
- Access specifiers
- Inheritance
- Static members
The main language-level difference is the default access:

  Declaration   Default member access
  struct      public
class       private

A struct is a natural choice for a simple record whose purpose is to
group related data.

Example:
struct Point {
    int x;
    int y;
};
The LogRecord follows the same data-oriented idea, but it groups
logging-specific fields.

Using struct does not mean the type cannot have behavior. It is a
design choice that communicates the type's role.
4. Type, Object, Member and Value
Consider:
LogRecord record;
- LogRecord is the type.
- record is the object name.
- The object contains the members declared in the struct.
The declaration creates an object. It does not automatically capture the
current time or current thread ID.

This distinction is important.

For example:
LogRecord record;
does not mean:
record.timestamp = std::chrono::system_clock::now();
record.thread_id = std::this_thread::get_id();
Those values must be assigned or supplied explicitly.
5. Aggregate Initialization
The current LogRecord is designed as an aggregate: it has public data
members and no user-declared constructors or other features that would
disqualify it as an aggregate under the C++17 aggregate rules.

That allows aggregate initialization:
LogRecord record{
    "Application started",
    LogLevel::INFO,
    std::chrono::system_clock::now(),
    std::this_thread::get_id()
};
What happens here?
The values initialize members in their declaration order:
1. "Application started" initializes message.
2. LogLevel::INFO initializes level.
3. system_clock::now() initializes timestamp.
4. this_thread::get_id() initializes thread_id.
The order matters. Aggregate initialization uses the order in which
members are declared.
Important distinction: aggregate initialization vs constructor
This:
LogRecord record{
    "Application started",
    LogLevel::INFO,
    std::chrono::system_clock::now(),
    std::this_thread::get_id()
};
is aggregate initialization for the current struct. It does not call a
custom LogRecord constructor because no such constructor has been
declared.

A class or struct with a user-declared constructor is initialized
through that constructor instead; it is no longer an aggregate under the
C++17 rules.
6. What If We Default-Initialize the Record?
Consider:
LogRecord record;
For this aggregate, the members are default-initialized:
- std::string message becomes an empty string.
- LogLevel level has no default member initializer and is not
  automatically set to one of the named severity values. Its value is
  indeterminate.
- timestamp is default-constructed as a system_clock::time_point
  representing its default time point, not the current time.
- thread_id is default-constructed as a default thread ID, not the
  current thread's ID.
Therefore, do not assume that merely creating a LogRecord captures
valid event metadata.

For the Logger, initialize all four members deliberately when creating a
record.
7. Understanding std::chrono::system_clock::time_point
What is system_clock?
std::chrono::system_clock is a clock intended to represent real-world
wall-clock time.

It can be used when an application needs a date/time timestamp.

Capture the current time with:
auto timestamp = std::chrono::system_clock::now();
What is time_point?
A time_point represents a particular point on a clock's timeline.

It is not itself a formatted date string such as:
2026-11-05 20:07:00
It is a C++ time value associated with a clock.
Why use system_clock for logging?
A log usually needs to show when an event occurred in real-world time.
system_clock is suitable for that purpose.

For elapsed-time measurement and benchmarking, a monotonic clock such as
std::chrono::steady_clock is generally the better choice.
Capture time explicitly
auto timestamp = std::chrono::system_clock::now();
This asks the system clock for its current time point.

The Logger will capture this value when it creates the record.
8. Understanding std::thread::id
A process can contain multiple threads.

Each thread has its own execution identity. C++ represents a thread ID
using:
std::thread::id
The current thread's ID can be obtained with:
std::this_thread::get_id()
For example:
auto currentThreadId = std::this_thread::get_id();
The LogRecord stores this ID so a log message can be associated with
the thread that created it.
Thread ID is not process ID
These identify different things:

  Identifier                     Identifies
  std::this_thread::get_id()   Current C++ thread
getpid() on POSIX/Linux      Current process

A process may contain multiple threads. Thread ID and process ID must
not be treated as interchangeable.

The textual representation of a std::thread::id is
implementation-dependent. Do not assume it is a portable numeric process
ID.
9. Where Is the LogRecord Object Created?
log_record.hpp defines the type. It does not need to create a real
event object itself.

The Logger will create a record when a logging request arrives.

Conceptually, inside the future Logger implementation:
LogRecord record{
    message,
    level,
    std::chrono::system_clock::now(),
    std::this_thread::get_id()
};
This is the point where the actual event data is captured.

The separation is:
- log_record.hpp --- defines the structure/type.
- logger.cpp --- will create and use actual LogRecord objects.
- log_formatter.cpp --- reads a record and converts it to text.
10. Member Initializer List: Related Concept
A constructor can initialize data members using a member initializer
list.

Example:
class Example {
public:
    Example(int value)
        : number(value)
    {
    }

private:
    int number;
};
The colon : introduces the member initializer list.

This is different from aggregate initialization:
LogRecord record{
    "Application started",
    LogLevel::INFO,
    std::chrono::system_clock::now(),
    std::this_thread::get_id()
};
The current LogRecord does not define a constructor, so the example
above is aggregate initialization, not a constructor member initializer
list.

For this project, keeping LogRecord as a simple aggregate is
intentional. It avoids adding a constructor that is not currently
needed.
11. Quick Reference
  Concept                             Meaning
  struct                            User-defined type; members are
public by default

  LogRecord                         Type that groups the data for one
log event

  record                            An object of type LogRecord

  Aggregate initialization            Initializes aggregate members in
declaration order

  std::string                       Stores the log message

  LogLevel                          Stores the severity

  system_clock::time_point          Represents a point on the system
clock timeline

  system_clock::now()               Captures the current system-clock
time point

  std::thread::id                   Represents a C++ thread ID

  std::this_thread::get_id()        Gets the current thread's ID
Member initializer list             Constructor syntax used to
initialize members after :
12. Common Mistakes to Avoid
1. Assuming LogRecord record; captures the current timestamp
   automatically.
2. Assuming a default-initialized LogLevel is automatically INFO or
   another named value.
3. Confusing a type name (LogRecord) with an object name (record).
4. Confusing aggregate initialization with a constructor call.
5. Forgetting that aggregate initialization follows member declaration
   order.
6. Confusing a thread ID with a process ID.
7. Treating a time_point as if it were already a formatted date/time
   string.
13. Progress
- [x] LogRecord struct declared.
- [x] Four event fields identified and reviewed.
- [x] Aggregate initialization discussed.
- [x] Declaration order and member initialization understood.
- [x] system_clock::time_point discussed.
- [x] std::this_thread::get_id() discussed.
- [x] Object creation responsibility identified as part of the future
  Logger implementation.
Next related topic: LogFormatter --- converting a LogRecord into
a readable string, including timestamp conversion and stream formatting.