# 14. CMake Build System and CTest

## 1. WHY — Why Do We Need a Build System?

A C++ project can contain multiple source files, header files, libraries, executables, and tests.

Consider our Logger project:

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
+-- examples/
|   +-- logger_example.cpp
|
+-- tests/
    +-- test_log_level.cpp
    +-- test_log_formatter.cpp
    +-- test_console_sink.cpp
    +-- test_file_sink.cpp
    +-- test_logger.cpp
```

As a project grows, compiling every source file manually becomes difficult.

For example, we would need to:

- Specify all source files.
- Specify include directories.
- Set the required C++ standard.
- Compile production code.
- Link object files.
- Build example executables.
- Build test executables.
- Execute registered tests.

A build system automates these tasks.

Our project uses **CMake** to configure and generate the build system.

## 2. Concept — What Is CMake?

CMake is a cross-platform build-system generator.

It reads project configuration from `CMakeLists.txt` and generates native build files for a selected build tool.

Examples of build tools include:

- GNU Make
- Ninja
- Microsoft Visual Studio build tools

In our Windows environment, we use:

```text
MinGW Makefiles
```

CMake configures the project and generates Makefiles that MinGW Make can execute.

### Important distinction

CMake is not itself the C++ compiler.

The responsibilities are different:

| Tool | Responsibility |
|---|---|
| CMake | Configures the project and generates build files |
| g++ | Compiles and links C++ code |
| GNU Make | Executes generated build instructions |
| CTest | Runs registered tests |

## 3. Mental Model — Build Process

```text
       CMakeLists.txt
              |
              v
        CMake Configure
              |
              v
       Build File Generation
              |
              v
          GNU Make
              |
              v
             g++
              |
       +------+------+
       |             |
       v             v
   Object Files   Libraries
       |             |
       +------+------+
              |
              v
         Executables
              |
              v
          CTest
```

This represents the general build and test workflow.

## 4. Understanding CMakeLists.txt

The root `CMakeLists.txt` describes how our project should be configured and built.

Our current configuration:

```cmake
cmake_minimum_required(VERSION 3.16)

project(Logger VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static Library
add_library(logger STATIC
    src/logger.cpp
    src/log_formatter.cpp
    src/console_sink.cpp
    src/file_sink.cpp
)

target_include_directories(logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Example Executable
add_executable(logger_example
    examples/logger_example.cpp
)

target_link_libraries(logger_example
    PRIVATE logger
)

# Test Executables
add_executable(test_log_level
    tests/test_log_level.cpp
)

target_link_libraries(test_log_level
    PRIVATE logger
)

add_executable(test_log_formatter
    tests/test_log_formatter.cpp
)

target_link_libraries(test_log_formatter
    PRIVATE logger
)

add_executable(test_console_sink
    tests/test_console_sink.cpp
)

target_link_libraries(test_console_sink
    PRIVATE logger
)

add_executable(test_file_sink
    tests/test_file_sink.cpp
)

target_link_libraries(test_file_sink
    PRIVATE logger
)

add_executable(test_logger
    tests/test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE logger
)

# Enable Testing
enable_testing()

# Register Tests
add_test(
    NAME LogLevelTest
    COMMAND test_log_level
)

add_test(
    NAME LogFormatterTest
    COMMAND test_log_formatter
)

add_test(
    NAME ConsoleSinkTest
    COMMAND test_console_sink
)

add_test(
    NAME FileSinkTest
    COMMAND test_file_sink
)

add_test(
    NAME LoggerTest
    COMMAND test_logger
)
```

## 5. Understanding CMake Commands

### 5.1 cmake_minimum_required()

```cmake
cmake_minimum_required(VERSION 3.16)
```

This specifies the minimum CMake version required by the project.

CMake checks the installed version against this requirement.

### 5.2 project()

```cmake
project(Logger VERSION 1.0.0 LANGUAGES CXX)
```

This defines the project.

- Project name: Logger
- Version: 1.0.0
- Language: C++

### 5.3 C++ Standard

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

These settings configure the required C++ language standard.

| Setting | Purpose |
|---|---|
| `CMAKE_CXX_STANDARD` | Requests C++17 |
| `CMAKE_CXX_STANDARD_REQUIRED` | Requires the requested standard |
| `CMAKE_CXX_EXTENSIONS` | Disables compiler-specific language extensions |

This ensures that the project is configured for standard C++17 rather than relying intentionally on compiler-specific extensions.

## 6. Understanding Static Libraries

Our Logger is built as a static library:

```cmake
add_library(logger STATIC
    src/logger.cpp
    src/log_formatter.cpp
    src/console_sink.cpp
    src/file_sink.cpp
)
```

A static library is a collection of compiled object code that can be linked into executable programs.

On our MinGW toolchain, the generated library normally has the filename:

```text
liblogger.a
```

The source files are compiled into object files, and the object code is packaged into the static library.

### Mental Model

```text
logger.cpp
     |
     v
logger.cpp.obj
     |
     |
log_formatter.cpp
     |
     v
log_formatter.cpp.obj
     |
     |
console_sink.cpp
     |
     v
console_sink.cpp.obj
     |
     |
file_sink.cpp
     |
     v
file_sink.cpp.obj
     |
     v
liblogger.a
```

The exact intermediate build paths are managed by CMake and the selected generator.

## 7. Understanding Source Files and Header Files

Our project separates declarations and implementations.

### Header Files

Located in:

```text
include/logger/
```

They contain declarations of classes, functions, and interfaces.

Examples:

```text
logger.hpp
log_level.hpp
log_record.hpp
log_formatter.hpp
log_sink.hpp
console_sink.hpp
file_sink.hpp
```

### Source Files

Located in:

```text
src/
```

They contain the implementations of the declared functions.

Examples:

```text
logger.cpp
log_formatter.cpp
console_sink.cpp
file_sink.cpp
```

This separation makes the library easier to organize and reuse.

## 8. Understanding target_include_directories()

```cmake
target_include_directories(logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

This tells CMake where consumers of the Logger library can find its public headers.

`CMAKE_CURRENT_SOURCE_DIR` represents the directory containing the current `CMakeLists.txt`.

The `PUBLIC` keyword means:

- The Logger target uses this include directory.
- Targets that link against Logger also receive this include directory.

This is why the example and test source files can write:

```cpp
#include "logger/logger.hpp"
```

without manually specifying the full header directory in every source file.

## 9. Building the Example Executable

```cmake
add_executable(logger_example
    examples/logger_example.cpp
)

target_link_libraries(logger_example
    PRIVATE logger
)
```

`add_executable()` defines an executable target.

`target_link_libraries()` connects that executable to the Logger library.

The example can now use the Logger classes and their implementations.

The `PRIVATE` keyword means that the Logger dependency is used by this executable target and is not propagated as a public dependency of the executable.

## 10. Building Test Executables

Each test file is built as a separate executable.

For example:

```cmake
add_executable(test_logger
    tests/test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE logger
)
```

This creates the `test_logger` executable and links it against the Logger static library.

Our project has five test executables:

| Executable | Source File |
|---|---|
| test_log_level | test_log_level.cpp |
| test_log_formatter | test_log_formatter.cpp |
| test_console_sink | test_console_sink.cpp |
| test_file_sink | test_file_sink.cpp |
| test_logger | test_logger.cpp |

Each executable can be run independently.

## 11. Understanding the CMake Build Directory

We use a separate build directory:

```text
09-logger/
|
+-- build/
+-- include/
+-- src/
+-- tests/
+-- examples/
+-- docs/
+-- CMakeLists.txt
```

The build directory contains generated build files and compiled outputs.

For example:

```text
build/
|
+-- CMakeCache.txt
+-- Makefile
+-- liblogger.a
+-- logger_example.exe
+-- test_log_level.exe
+-- test_log_formatter.exe
+-- test_console_sink.exe
+-- test_file_sink.exe
+-- test_logger.exe
```

Exact intermediate files and folder structure depend on the generator and build configuration.

Keeping generated files separate from source code makes the project cleaner.

## 12. Understanding the Configure Command

We use:

```bash
cmake -S . -B build -G "MinGW Makefiles"
```

This command configures the project and generates the build system.

| Option | Meaning |
|---|---|
| `-S .` | Current directory is the source directory |
| `-B build` | Use the build directory |
| `-G` | Select a build-system generator |
| `MinGW Makefiles` | Generate Makefiles for MinGW |

The command does not itself compile every C++ source file.

It configures and generates the build instructions.

## 13. Understanding the Build Command

```bash
cmake --build build
```

This asks CMake to invoke the selected native build tool using the generated build files.

In our case, the selected generator is MinGW Makefiles.

The build process compiles the source files and links the required outputs.

Our successful build output included:

```text
[ 29%] Built target logger
[ 41%] Built target logger_example
[ 52%] Built target test_log_level
[ 64%] Built target test_log_formatter
[ 76%] Built target test_console_sink
[ 88%] Built target test_file_sink
[100%] Built target test_logger
```

This confirms that all listed targets were built successfully.

## 14. Understanding CTest

CTest is CMake's test execution tool.

It runs tests that have been registered using `add_test()`.

Before registering tests, we enable testing:

```cmake
enable_testing()
```

Then we register each test:

```cmake
add_test(
    NAME LoggerTest
    COMMAND test_logger
)
```

The `NAME` field identifies the test.

The `COMMAND` field specifies the executable or command to run.

CTest does not automatically discover every executable in the build directory. Tests must be registered.

## 15. Running CTest

Our complete test command is:

```bash
ctest --test-dir build --output-on-failure
```

### Understanding the Options

`--test-dir build`

Specifies the directory containing the generated test configuration.

`--output-on-failure`

Displays additional output from tests that fail.

Our five registered tests are:

1. LogLevelTest
2. LogFormatterTest
3. ConsoleSinkTest
4. FileSinkTest
5. LoggerTest

## 16. Actual Build and Test Results

On November 6, 2026, we successfully configured and built the Logger project.

The build generated the Logger static library, example executable, and all five test executables.

The CTest result was:

```text
Test project ...

1/5 Test #1: LogLevelTest ............ Passed
2/5 Test #2: LogFormatterTest ........ Passed
3/5 Test #3: ConsoleSinkTest ......... Passed
4/5 Test #4: FileSinkTest ............ Passed
5/5 Test #5: LoggerTest .............. Passed

100% tests passed, 0 tests failed out of 5
```

This is evidence that all five registered test executables passed in that recorded run.

## 17. Complete Build Workflow

Our normal project workflow is:

### Step 1 — Configure

```bash
cmake -S . -B build -G "MinGW Makefiles"
```

### Step 2 — Build

```bash
cmake --build build
```

### Step 3 — Run Example

```bash
.\build\logger_example.exe
```

### Step 4 — Run One Test

```bash
.\build\test_logger.exe
```

### Step 5 — Run All Tests

```bash
ctest --test-dir build --output-on-failure
```

This workflow allows us to configure, compile, execute, and validate the project consistently.

## 18. Important Engineering Points

- CMake generates native build-system files.
- CMake is not the C++ compiler.
- g++ compiles and links C++ code.
- GNU Make executes generated build instructions.
- C++17 is configured as the project standard.
- The Logger is built as a static library.
- `target_include_directories()` configures header search paths.
- `target_link_libraries()` connects targets to libraries.
- `PRIVATE` and `PUBLIC` describe dependency visibility.
- Test executables are separate build targets.
- `enable_testing()` enables CTest integration.
- `add_test()` registers test commands.
- CTest runs registered tests.
- Build artifacts are kept separate from source files.

## 19. Quick Reference

| Command or CMake Function | Purpose |
|---|---|
| `cmake_minimum_required()` | Sets minimum CMake version |
| `project()` | Defines project metadata |
| `set()` | Configures CMake variables |
| `add_library()` | Creates a library target |
| `add_executable()` | Creates an executable target |
| `target_include_directories()` | Configures include directories |
| `target_link_libraries()` | Links target dependencies |
| `enable_testing()` | Enables testing |
| `add_test()` | Registers a test |
| `cmake -S . -B build` | Configures a project |
| `cmake --build build` | Builds configured targets |
| `ctest` | Executes registered tests |

## 20. Progress

**Completed:**

- CMake project configuration.
- C++17 configuration.
- Static library creation.
- Public include directory configuration.
- Example executable creation.
- Five test executable targets.
- Library linking.
- CTest registration.
- Configure and build workflow.
- Successful project build.
- Successful 5/5 CTest run.

**Next Topic:**

`15-performance-benchmarking-and-analysis.md`

We will study how to measure Logger performance, understand throughput and latency, design meaningful benchmarks, and interpret actual benchmark results without making unsupported performance claims.