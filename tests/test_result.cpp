#include <cassert>
#include <string>

#include "result.hpp"

int main() {
  // ok path
  auto r1 = Result<int, std::string>::ok(42);
  assert(r1);
  assert(r1.value() == 42);

  // err path
  auto r2 = Result<int, std::string>::err("oops");
  assert(!r2);
  assert(r2.error() == "oops");

  // move value out
  auto r3 = Result<std::string, int>::ok("hello");
  std::string s = std::move(r3).value();
  assert(s == "hello");

  // T == E — 编译不歧义（OkTag/ErrTag 区分）
  auto r4 = Result<int, int>::ok(1);
  auto r5 = Result<int, int>::err(2);
  assert(r4);
  assert(!r5);
  assert(r4.value() == 1);
  assert(r5.error() == 2);
}
