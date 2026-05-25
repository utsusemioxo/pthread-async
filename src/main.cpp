#include <chrono>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "async_task.hpp"

using Ri = Result<int, std::string>;
using Rll = Result<long long, std::string>;

Ri safe_divide(int a, int b) {
  if (b == 0) return Ri::err("division by zero");
  return Ri::ok(a / b);
}

int main() {
  // ── 1. 基本 spawn ────────────────────────────────────────────────────────
  std::cout << "=== 1. basic spawn ===\n";
  auto r = spawn([] { return Ri::ok(6 * 7); });
  if (r) {
    auto res = r.value().get();
    std::cout << "6 * 7 = " << (res ? std::to_string(res.value()) : res.error()) << "\n";
  }

  // ── 2. 任务失败通过 Result 传递 ───────────────────────────────────────────
  std::cout << "\n=== 2. task error via Result ===\n";
  auto d1 = safe_divide(10, 2);
  auto d2 = safe_divide(10, 0);
  std::cout << "10/2 = " << (d1 ? std::to_string(d1.value()) : d1.error()) << "\n";
  std::cout << "10/0 = " << (d2 ? std::to_string(d2.value()) : d2.error()) << "\n";

  // ── 3. 并发 map ───────────────────────────────────────────────────────────
  std::cout << "\n=== 3. parallel map ===\n";
  std::vector<int> input{1, 2, 3, 4, 5, 6, 7, 8};
  std::vector<std::future<Ri>> futs;
  futs.reserve(input.size());

  for (int x : input) {
    auto r2 = spawn([x] { return Ri::ok(x * x); });
    if (!r2) { std::cerr << "spawn failed: " << r2.error() << "\n"; return 1; }
    futs.push_back(std::move(r2).value());
  }

  std::cout << "squares:";
  for (auto& f : futs) {
    auto res = f.get();
    std::cout << " " << (res ? std::to_string(res.value()) : res.error());
  }
  std::cout << "\n";

  // ── 4. 并发 reduce ────────────────────────────────────────────────────────
  std::cout << "\n=== 4. parallel reduce ===\n";
  std::vector<int> data(1000);
  std::iota(data.begin(), data.end(), 1);

  constexpr int CHUNKS = 4;
  const int chunk = static_cast<int>(data.size()) / CHUNKS;
  std::vector<std::future<Rll>> sums;

  for (int i = 0; i < CHUNKS; ++i) {
    int lo = i * chunk;
    int hi = (i == CHUNKS - 1) ? static_cast<int>(data.size()) : lo + chunk;
    auto rs = spawn([&data, lo, hi] {
      return Rll::ok(std::accumulate(data.begin() + lo, data.begin() + hi, 0LL));
    });
    if (!rs) { std::cerr << "spawn failed\n"; return 1; }
    sums.push_back(std::move(rs).value());
  }

  long long total = 0;
  for (auto& f : sums) {
    auto res = f.get();
    if (res) total += res.value();
  }
  std::cout << "sum(1..1000) = " << total << "  (expected 500500)\n";

  // ── 5. move-only lambda（转发引用修复的受益者）──────────────────────────
  std::cout << "\n=== 5. move-only capture ===\n";
  auto ptr = std::make_unique<int>(1234);
  auto r5 = spawn([p = std::move(ptr)] { return Ri::ok(*p); });
  if (r5) {
    auto res = r5.value().get();
    std::cout << "captured unique_ptr value = " << res.value() << "\n";
  }
}
