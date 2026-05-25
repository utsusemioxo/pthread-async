#pragma once
#include <utility>
#include <variant>

template <typename T, typename E>
class [[nodiscard]] Result {
  struct OkTag {};
  struct ErrTag {};

  std::variant<T, E> data_;

  Result(OkTag, T v) : data_(std::in_place_index<0>, std::move(v)) {}
  Result(ErrTag, E e) : data_(std::in_place_index<1>, std::move(e)) {}

 public:
  static Result ok(T v) { return Result(OkTag{}, std::move(v)); }
  static Result err(E e) { return Result(ErrTag{}, std::move(e)); }

  explicit operator bool() const noexcept { return data_.index() == 0; }

  T& value() & { return std::get<0>(data_); }
  const T& value() const& { return std::get<0>(data_); }
  T&& value() && { return std::get<0>(std::move(data_)); }

  E& error() & { return std::get<1>(data_); }
  const E& error() const& { return std::get<1>(data_); }
  E&& error() && { return std::get<1>(std::move(data_)); }
};
