#include <atomic>
#include <cassert>
#include <memory>
#include <string>
#include <unistd.h>

#include "async_task.hpp"

using Ri = Result<int, std::string>;
using Rv = Result<bool, std::string>;  // void 替代品：用 bool 占位

int main() {
  // 基本返回值
  auto r1 = spawn([]() noexcept { return Ri::ok(42); });
  assert(r1);
  auto joined1 = r1.value().join();
  assert(joined1);
  auto inner1 = std::move(joined1).value();
  assert(inner1);
  assert(inner1.value() == 42);

  // 任务失败路径
  auto r2 = spawn([]() noexcept { return Ri::err(std::string("bad input")); });
  assert(r2);
  auto joined2 = r2.value().join();
  assert(joined2);
  auto inner2 = std::move(joined2).value();
  assert(!inner2);
  assert(inner2.error() == "bad input");

  // 无返回值任务（用 Result<bool,E> 代替 void）
  std::atomic<bool> ran{false};
  auto r3 = spawn([&ran]() noexcept {
    ran = true;
    return Rv::ok(true);
  });
  assert(r3);
  auto joined3 = r3.value().join();
  assert(joined3);
  assert(joined3.value());
  assert(ran);

  // 多个并发任务
  constexpr int N = 8;
  AsyncTask<Ri> tasks[N];
  for (int i = 0; i < N; ++i) {
    auto r = spawn([i]() noexcept { return Ri::ok(i * i); });
    assert(r);
    tasks[i] = std::move(r).value();
  }
  for (int i = 0; i < N; ++i) {
    auto joined = tasks[i].join();
    assert(joined);
    auto res = std::move(joined).value();
    assert(res && res.value() == i * i);
  }

  // move-only lambda（转发引用修复的受益者）
  auto ptr = std::make_unique<int>(99);
  auto r4 = spawn([p = std::move(ptr)]() noexcept { return Ri::ok(*p); });
  assert(r4);
  auto joined4 = r4.value().join();
  assert(joined4);
  assert(joined4.value().value() == 99);

  // join() 是 pthread 资源回收和任务完成同步点
  std::atomic<bool> finished{false};
  auto r5 = spawn([&finished]() noexcept {
    usleep(20 * 1000);
    finished = true;
    return Rv::ok(true);
  });
  assert(r5);
  assert(!finished);
  auto joined5 = r5.value().join();
  assert(joined5);
  auto inner5 = std::move(joined5).value();
  assert(inner5);
  assert(finished);

  // join() 只能成功一次
  auto joined_again = r5.value().join();
  assert(!joined_again);
}
