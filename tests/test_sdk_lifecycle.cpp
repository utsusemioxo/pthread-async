#include <atomic>
#include <cassert>
#include <unistd.h>

#include "async_task.hpp"

using Rb = Result<bool, int>;

class FakeSdk {
 public:
  Rb init() {
    released_ = false;
    sub_init_done_ = false;
    sub_uninit_done_ = false;

    auto r = spawn([this]() noexcept {
      usleep(30 * 1000);
      if (released_) return Rb::err(1);
      sub_init_done_ = true;
      return Rb::ok(true);
    });
    if (!r) return Rb::err(r.error());

    init_task_ = std::move(r).value();
    return Rb::ok(true);
  }

  Rb uninit() {
    if (init_task_.joinable()) {
      auto joined = init_task_.join();
      if (!joined) return Rb::err(joined.error());
      auto init_result = std::move(joined).value();
      if (!init_result) return init_result;
    }

    auto r = spawn([this]() noexcept {
      if (released_ || !sub_init_done_) return Rb::err(2);
      sub_uninit_done_ = true;
      return Rb::ok(true);
    });
    if (!r) return Rb::err(r.error());

    auto joined = std::move(r).value().join();
    if (!joined) return Rb::err(joined.error());
    auto uninit_result = std::move(joined).value();
    if (!uninit_result) return uninit_result;

    released_ = true;
    return Rb::ok(true);
  }

  bool sub_init_done() const { return sub_init_done_; }
  bool sub_uninit_done() const { return sub_uninit_done_; }
  bool released() const { return released_; }

 private:
  AsyncTask<Rb> init_task_;
  std::atomic<bool> released_{false};
  std::atomic<bool> sub_init_done_{false};
  std::atomic<bool> sub_uninit_done_{false};
};

int main() {
  FakeSdk sdk;

  auto init_result = sdk.init();
  assert(init_result);

  auto uninit_result = sdk.uninit();
  assert(uninit_result);
  assert(sdk.sub_init_done());
  assert(sdk.sub_uninit_done());
  assert(sdk.released());
}
