# pthread-async

Exception-free async tasks in C++17 using pthread + `Result<T,E>`.

## Build

```bash
mkdir build && cd build
cmake .. && make
# 或直接编译单文件
g++ -std=c++17 -lpthread -o main main.cpp
Architecture
include/
  result.hpp       # Result<T,E> — 基于 std::variant，无异常错误类型
  async_task.hpp   # spawn() — pthread + AsyncTask join handle
src/
  main.cpp         # 示例与集成测试
tests/
  test_result.cpp  # Result 单元测试
  test_async.cpp   # async 任务测试
Core Design
Result<T, E> — 所有错误通过返回值传递，禁止 throw/catch：

Result::ok(v) / Result::err(e) 构造

operator bool() 判断成功

.value() / .error() 取值

spawn(F func) — 用 pthread 启动异步任务：

返回 Result<AsyncTask<R>, int>（int 为 pthread errno）

内部用 unique_ptr 管理 Payload，用 TaskState 保存返回值

线程不 detach，由 AsyncTask::join() 调用 pthread_join 回收资源

Key Constraints

C++17，不使用 std::thread

不使用异常（no throw/catch），错误一律用 Result 传递

不引入第三方库

线程函数签名必须是 void*(*)(void*) 才能传给 pthread_create

Commands
# 编译并运行示例
g++ -std=c++17 -Iinclude -lpthread src/main.cpp -o main && ./main

# 编译测试
g++ -std=c++17 -Iinclude -lpthread tests/test_result.cpp -o test_result && ./test_result
