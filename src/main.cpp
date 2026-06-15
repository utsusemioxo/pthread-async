#include <iostream>
#include <string>
#include <unistd.h>

#include "async_task.hpp"

using Ri = Result<int, std::string>;

class Worker {
 public:
  // foo：发起任务，立即返回
  Result<bool, std::string> foo() {
    auto r = spawn([]() noexcept {
      usleep(50 * 1000);
      return Ri::ok(42);
    });
    if (!r) {
      return Result<bool, std::string>::err("spawn failed: " + std::to_string(r.error()));
    }
    task_ = std::move(r).value();
    return Result<bool, std::string>::ok(true);
  }

  // bar：等任务结束，取结果
  Ri bar() {
    if (!task_.joinable()) return Ri::err("no task running");
    auto joined = task_.join();
    if (!joined) return Ri::err("join failed: " + std::to_string(joined.error()));
    return std::move(joined).value();
  }

 private:
  AsyncTask<Ri> task_;
};

int main() {
  Worker w;

  auto r = w.foo();
  if (!r) { std::cerr << r.error() << "\n"; return 1; }

  std::cout << "task spawned, doing other work...\n";

  auto res = w.bar();   // pthread_join 后取结果
  if (res) std::cout << "result = " << res.value() << "\n";
  else     std::cerr << "error  = " << res.error()  << "\n";
}
