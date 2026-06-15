#pragma once
#include <pthread.h>

#include <cerrno>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "result.hpp"

namespace detail {

template <typename R>
class TaskState {
 public:
  TaskState() = default;
  TaskState(const TaskState&) = delete;
  TaskState& operator=(const TaskState&) = delete;

  ~TaskState() { destroy(); }

  void set(R&& value) noexcept {
    new (&storage_) R(std::move(value));
    ready_ = true;
  }

  bool ready() const noexcept { return ready_; }

  R take() noexcept {
    R value = std::move(*ptr());
    destroy();
    return value;
  }

 private:
  R* ptr() noexcept { return reinterpret_cast<R*>(&storage_); }

  void destroy() noexcept {
    if (ready_) {
      ptr()->~R();
      ready_ = false;
    }
  }

  typename std::aligned_storage<sizeof(R), alignof(R)>::type storage_;
  bool ready_{false};
};

// F 直接存储，不经过 std::function，支持 move-only lambda
template <typename F, typename FR>
struct Payload {
  F fn;
  TaskState<FR>* state;

  Payload(F&& f, TaskState<FR>* s) noexcept(std::is_nothrow_constructible_v<F, F&&>)
      : fn(std::forward<F>(f)), state(s) {}
  Payload(const Payload&) = delete;
  Payload& operator=(const Payload&) = delete;
};

template <typename F, typename FR>
void* thread_fn(void* arg) noexcept {
  std::unique_ptr<Payload<F, FR>> p(static_cast<Payload<F, FR>*>(arg));
  p->state->set(p->fn());
  return nullptr;
}

}  // namespace detail

template <typename R>
class [[nodiscard]] AsyncTask {
 public:
  AsyncTask() = default;
  AsyncTask(const AsyncTask&) = delete;
  AsyncTask& operator=(const AsyncTask&) = delete;

  AsyncTask(AsyncTask&& other) noexcept
      : tid_(other.tid_), state_(other.state_), joinable_(other.joinable_),
        consumed_(other.consumed_) {
    other.state_ = nullptr;
    other.joinable_ = false;
    other.consumed_ = true;
  }

  AsyncTask& operator=(AsyncTask&& other) noexcept {
    if (this == &other) return *this;
    cleanup();
    tid_ = other.tid_;
    state_ = other.state_;
    joinable_ = other.joinable_;
    consumed_ = other.consumed_;
    other.state_ = nullptr;
    other.joinable_ = false;
    other.consumed_ = true;
    return *this;
  }

  ~AsyncTask() { cleanup(); }

  bool joinable() const noexcept { return joinable_; }

  Result<R, int> join() {
    if (!state_ || consumed_) return Result<R, int>::err(EINVAL);

    if (joinable_) {
      int err = pthread_join(tid_, nullptr);
      if (err != 0) return Result<R, int>::err(err);
      joinable_ = false;
    }

    if (!state_->ready()) return Result<R, int>::err(EIO);

    R value = state_->take();
    delete state_;
    state_ = nullptr;
    consumed_ = true;
    return Result<R, int>::ok(std::move(value));
  }

 private:
  AsyncTask(pthread_t tid, detail::TaskState<R>* state) noexcept
      : tid_(tid), state_(state), joinable_(true), consumed_(false) {}

  void cleanup() noexcept {
    if (joinable_) {
      pthread_join(tid_, nullptr);
      joinable_ = false;
    }
    delete state_;
    state_ = nullptr;
    consumed_ = true;
  }

  pthread_t tid_{};
  detail::TaskState<R>* state_{nullptr};
  bool joinable_{false};
  bool consumed_{true};

  template <typename F, typename FR>
  friend Result<AsyncTask<FR>, int> spawn(F&& func);
};

// spawn(F) — F 必须返回 Result<T,E>
// 返回 Result<AsyncTask<Result<T,E>>, int>
//   外层 Result：pthread_create 是否成功（errno）
//   AsyncTask::join()：pthread_join 是否成功（errno）
//   内层 Result：任务本身是否成功
template <typename F, typename FR = std::invoke_result_t<std::decay_t<F>&>>
Result<AsyncTask<FR>, int> spawn(F&& func) {
  using DecayF = std::decay_t<F>;

  static_assert(std::is_nothrow_invocable_v<DecayF&>,
                "spawn() tasks must be noexcept and return errors via Result");
  static_assert(std::is_nothrow_constructible_v<DecayF, F&&>,
                "spawn() task objects must be nothrow-move/copy constructible");
  static_assert(std::is_nothrow_destructible_v<DecayF>,
                "spawn() task objects must be nothrow destructible");
  static_assert(std::is_nothrow_move_constructible_v<FR>,
                "spawn() task results must be nothrow move constructible");
  static_assert(std::is_nothrow_destructible_v<FR>,
                "spawn() task results must be nothrow destructible");

  auto* state = new (std::nothrow) detail::TaskState<FR>();
  if (state == nullptr) return Result<AsyncTask<FR>, int>::err(ENOMEM);

  auto* p = new (std::nothrow) detail::Payload<DecayF, FR>(std::forward<F>(func), state);
  if (p == nullptr) {
    delete state;
    return Result<AsyncTask<FR>, int>::err(ENOMEM);
  }

  pthread_t tid;
  int err = pthread_create(&tid, nullptr, detail::thread_fn<DecayF, FR>, p);
  if (err != 0) {
    delete p;
    delete state;
    return Result<AsyncTask<FR>, int>::err(err);
  }

  return Result<AsyncTask<FR>, int>::ok(AsyncTask<FR>(tid, state));
}
