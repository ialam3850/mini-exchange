#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace mx {

// Grows by chunks when the free list runs out.
template <typename T>
class ObjectPool {
 public:
  explicit ObjectPool(std::size_t chunk_size = 64 * 1024) : chunk_size_(chunk_size) { grow(); }

  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;

  template <typename... Args>
  T* allocate(Args&&... args) {
#ifdef MX_DISABLE_POOL
    ++live_;
    return new T(std::forward<Args>(args)...);
#endif
    if (!free_) grow();
    Node* n = free_;
    free_ = n->next;
    ++live_;
    return new (n->storage) T(std::forward<Args>(args)...);
  }

  void release(T* p) {
#ifdef MX_DISABLE_POOL
    --live_;
    delete p;
    return;
#endif
    p->~T();
    Node* n = reinterpret_cast<Node*>(p);
    n->next = free_;
    free_ = n;
    --live_;
  }

  std::size_t live() const { return live_; }
  std::size_t capacity() const { return chunks_.size() * chunk_size_; }

 private:
  union Node {
    Node* next;
    alignas(T) unsigned char storage[sizeof(T)];
  };

  void grow() {
    auto chunk = std::make_unique<Node[]>(chunk_size_);
    for (std::size_t i = 0; i < chunk_size_; ++i) {
      chunk[i].next = (i + 1 < chunk_size_) ? &chunk[i + 1] : free_;
    }
    free_ = &chunk[0];
    chunks_.push_back(std::move(chunk));
  }

  std::size_t chunk_size_;
  std::vector<std::unique_ptr<Node[]>> chunks_;
  Node* free_ = nullptr;
  std::size_t live_ = 0;
};

}
