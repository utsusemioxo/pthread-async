# pthread-async

Exception-free async tasks in C++17 using `pthread` + `Result<T,E>`.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/main
```

## Test

```bash
cd build && ctest --output-on-failure
```

## Usage

```cpp
#include "async_task.hpp"

// spawn 返回 Result<AsyncTask<R>, int>
auto r = spawn([]() noexcept { return Result<int, std::string>::ok(42); });
if (!r) {
    std::cerr << "pthread_create failed: " << r.error() << "\n";
    return;
}
auto joined = r.value().join(); // pthread_join 后取任务结果
if (!joined) {
    std::cerr << "pthread_join failed: " << joined.error() << "\n";
    return;
}
auto result = std::move(joined).value();
```

## Architecture & Ownership

```mermaid
sequenceDiagram
    participant Caller
    participant Spawn as spawn()
    participant Task as AsyncTask<R>
    participant State as TaskState<R>
    participant Payload
    participant Thread as pthread worker

    Caller->>Spawn: pass noexcept callable F
    Spawn->>State: allocate result storage
    Spawn->>Payload: allocate callable + State pointer
    Spawn->>Thread: pthread_create(thread_fn, Payload*)
    Spawn-->>Caller: Result<AsyncTask<R>, int>
    Note over Caller,Task: Caller owns AsyncTask<R>
    Note over Task,State: AsyncTask owns TaskState<R>
    Note over Thread,Payload: Worker owns Payload via unique_ptr
    Thread->>Payload: run callable
    Payload->>State: placement-new stores R
    Thread-->>Thread: Payload destroyed at thread exit
    Caller->>Task: join()
    Task->>Thread: pthread_join()
    Task->>State: move result out
    Task-->>Caller: Result<R, int>
    Note over Task,State: TaskState destroyed after result is consumed
```

Ownership rules:

- `spawn()` allocates `TaskState<R>` and `Payload`. If allocation or `pthread_create` fails, it frees what it allocated and returns `Result::err(errno)`.
- After `pthread_create` succeeds, the worker thread owns `Payload`; `thread_fn` wraps it in `std::unique_ptr` and destroys it when the task exits.
- The returned `AsyncTask<R>` owns `TaskState<R>` and the joinable `pthread_t`.
- `AsyncTask::join()` calls `pthread_join()`, moves the stored result out of `TaskState<R>`, destroys `TaskState<R>`, and returns `Result<R, int>`.
- If an `AsyncTask<R>` is destroyed before `join()`, its destructor joins the thread and frees internal storage, but the task result is discarded.

```cpp
// Result<T,E> — 错误通过返回值传递，不使用异常
Result<int, std::string> safe_div(int a, int b) {
    if (b == 0) return Result<int, std::string>::err("division by zero");
    return Result<int, std::string>::ok(a / b);
}

auto r = safe_div(10, 0);
if (r) std::cout << r.value();
else   std::cerr << r.error();
```

## Constraints

- C++17，不使用 `std::thread`
- 不使用异常，错误一律通过 `Result` 返回
- 不引入第三方库
