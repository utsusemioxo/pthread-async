# Repository Guidelines

## Project Structure & Module Organization

This is a small C++17 library-style project for exception-free async work with `pthread` and `Result<T,E>`.

- `include/result.hpp`: `Result<T,E>` value/error container built on `std::variant`.
- `include/async_task.hpp`: `spawn()` implementation using `pthread` and a no-exception `AsyncTask` result handle.
- `src/main.cpp`: runnable example and integration-style demonstration.
- `tests/test_result.cpp`: unit coverage for `Result`.
- `tests/test_async.cpp`: unit coverage for async task behavior.
- `CMakeLists.txt`: build graph for the example binary and tests.

Generated build output belongs in `build/` and should not be treated as source.

## Build, Test, and Development Commands

```bash
cmake -S . -B build -G Ninja
```
Configure the project and generate Ninja build files.

```bash
cmake --build build
```
Builds the `main`, `test_result`, and `test_async` executables with Ninja.

```bash
./build/main
```
Runs the sample program.

```bash
cd build && ctest --output-on-failure
```
Runs all registered tests and prints failing output.

## Coding Style & Naming Conventions

Use C++17 only. The project is built with exceptions disabled on Clang/GCC: do not add `throw`/`catch`; return errors through `Result<T,E>`. Do not replace `pthread` with `std::thread`, and avoid third-party dependencies.

Follow the existing style: two-space indentation, braces on the same line, concise helper names, and `snake_case` for functions and local variables. Template parameters use short CamelCase names such as `FR` or `DecayF` when they mirror nearby code.

Public headers should stay self-contained and include what they use. Prefer move-aware code paths because `spawn()` supports move-only callables.

## Testing Guidelines

Tests are simple C++ executables registered with CTest and use `assert`. Name new test files `tests/test_<feature>.cpp`, add a matching executable and `add_test()` entry in `CMakeLists.txt`, then run:

```bash
cmake -S . -B build -G Ninja
cmake --build build
cd build && ctest --output-on-failure
```

Cover both success and error paths. For async changes, include cases for failed task results, multiple concurrent tasks, and move-only lambdas where relevant.

## Commit & Pull Request Guidelines

The current history uses concise imperative summaries, for example: `Initial implementation of exception-free async task system`. Keep commit subjects short, descriptive, and focused on one change.

Pull requests should include a brief description, the commands run to verify the change, and any relevant behavioral notes. Link issues when applicable. Screenshots are not needed unless a future change adds visual output.
