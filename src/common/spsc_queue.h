#pragma once
#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
namespace mx {
inline constexpr std::size_t kCacheLine = 64;

// One producer and one consumer.
template<class T> class SpscQueue {
  static_assert(std::is_nothrow_move_constructible_v<T>);
  struct Slot {
    alignas(T) unsigned char bytes[sizeof(T)];
    T* ptr() { return std::launder(reinterpret_cast<T*>(bytes)); }
  };
 public:
  explicit SpscQueue(std::size_t requested)
      : capacity_(round_capacity(requested)), slots_(std::make_unique<Slot[]>(capacity_)) {}
  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;
  ~SpscQueue() { while (front()) pop(); }
  template<class... Args> bool try_emplace(Args&&... args) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (head_ - tail_ == capacity_) return false;
    new (slots_[head_ % capacity_].ptr()) T(std::forward<Args>(args)...);
    ++head_;
    return true;
  }
  bool try_push(const T& v) { return try_emplace(v); }
  bool try_push(T&& v) { return try_emplace(std::move(v)); }
  T* front() {
    std::lock_guard<std::mutex> lock(mutex_);
    return head_ == tail_ ? nullptr : slots_[tail_ % capacity_].ptr();
  }
  void pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    slots_[tail_ % capacity_].ptr()->~T();
    ++tail_;
  }
  std::optional<T> try_pop() {
    T* item = front();
    if (!item) return std::nullopt;
    std::optional<T> out(std::move(*item));
    pop();
    return out;
  }
  std::size_t capacity() const { return capacity_; }
  std::size_t size_approx() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return head_ - tail_;
  }
 private:
  static std::size_t round_capacity(std::size_t n) {
    if (n < 2) n = 2;
    std::size_t p = 2;
    while (p < n) p *= 2;
    return p;
  }
  const std::size_t capacity_;
  std::unique_ptr<Slot[]> slots_;
  mutable std::mutex mutex_;
  std::size_t head_ = 0, tail_ = 0;
};
}
