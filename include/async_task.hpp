#pragma once
#include <pthread.h>

#include <future>
#include <memory>
#include <type_traits>

#include "result.hpp"

namespace detail {

// F 直接存储，不经过 std::function，支持 move-only lambda
template <typename F, typename FR>
struct Payload {
  F fn;
  std::promise<FR> promise;

  explicit Payload(F&& f) : fn(std::forward<F>(f)) {}
  Payload(const Payload&) = delete;
  Payload& operator=(const Payload&) = delete;
};

template <typename F, typename FR>
void* thread_fn(void* arg) {
  std::unique_ptr<Payload<F, FR>> p(static_cast<Payload<F, FR>*>(arg));
  p->promise.set_value(p->fn());
  return nullptr;
}

}  // namespace detail

// spawn(F) — F 必须返回 Result<T,E>
// 返回 Result<future<Result<T,E>>, int>
//   外层 Result：pthread_create 是否成功（errno）
//   内层 Result：任务本身是否成功
template <typename F, typename FR = std::invoke_result_t<std::decay_t<F>>>
Result<std::future<FR>, int> spawn(F&& func) {
  using DecayF = std::decay_t<F>;
  auto p = std::make_unique<detail::Payload<DecayF, FR>>(std::forward<F>(func));
  auto fut = p->promise.get_future();

  pthread_t tid;
  int err = pthread_create(&tid, nullptr, detail::thread_fn<DecayF, FR>, p.get());
  if (err != 0) return Result<std::future<FR>, int>::err(err);

  p.release();  // 所有权移交 thread_fn
  pthread_detach(tid);

  return Result<std::future<FR>, int>::ok(std::move(fut));
}
