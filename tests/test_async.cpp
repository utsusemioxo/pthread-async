#include <atomic>
#include <cassert>
#include <chrono>
#include <string>

#include "async_task.hpp"

using Ri = Result<int, std::string>;
using Rv = Result<bool, std::string>;  // void 替代品：用 bool 占位

int main() {
  // 基本返回值
  auto r1 = spawn([] { return Ri::ok(42); });
  assert(r1);
  auto inner1 = r1.value().get();
  assert(inner1);
  assert(inner1.value() == 42);

  // 任务失败路径
  auto r2 = spawn([] { return Ri::err(std::string("bad input")); });
  assert(r2);
  auto inner2 = r2.value().get();
  assert(!inner2);
  assert(inner2.error() == "bad input");

  // 无返回值任务（用 Result<bool,E> 代替 void）
  std::atomic<bool> ran{false};
  auto r3 = spawn([&ran] {
    ran = true;
    return Rv::ok(true);
  });
  assert(r3);
  assert(r3.value().get());
  assert(ran);

  // 多个并发任务
  constexpr int N = 8;
  std::future<Ri> futs[N];
  for (int i = 0; i < N; ++i) {
    auto r = spawn([i] { return Ri::ok(i * i); });
    assert(r);
    futs[i] = std::move(r).value();
  }
  for (int i = 0; i < N; ++i) {
    auto res = futs[i].get();
    assert(res && res.value() == i * i);
  }

  // move-only lambda（转发引用修复的受益者）
  auto ptr = std::make_unique<int>(99);
  auto r4 = spawn([p = std::move(ptr)] { return Ri::ok(*p); });
  assert(r4);
  assert(r4.value().get().value() == 99);
}
