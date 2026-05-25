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

// spawn 返回 Result<std::future<R>, int>
auto r = spawn([] { return 42; });
if (!r) {
    std::cerr << "pthread_create failed: " << r.error() << "\n";
    return;
}
int result = r.value().get();   // 阻塞直到任务完成
```

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
