#include <gtest/gtest.h>

#include <memory>
#include <thread>

#include "common/spsc_queue.h"

using mx::SpscQueue;

TEST(SpscQueue, CapacityRoundsUpToPowerOfTwo) {
  SpscQueue<int> q(5);
  EXPECT_EQ(q.capacity(), 8u);
}

TEST(SpscQueue, FullAndEmpty) {
  SpscQueue<int> q(4);
  EXPECT_EQ(q.front(), nullptr);
  for (int i = 0; i < 4; ++i) EXPECT_TRUE(q.try_push(i));
  EXPECT_FALSE(q.try_push(99));
  for (int i = 0; i < 4; ++i) {
    auto v = q.try_pop();
    ASSERT_TRUE(v);
    EXPECT_EQ(*v, i);
  }
  EXPECT_FALSE(q.try_pop());
}

TEST(SpscQueue, DestroysRemainingElements) {
  auto tracker = std::make_shared<int>(0);
  {
    SpscQueue<std::shared_ptr<int>> q(8);
    q.try_push(tracker);
    q.try_push(tracker);
    EXPECT_EQ(tracker.use_count(), 3);
  }
  EXPECT_EQ(tracker.use_count(), 1);
}

TEST(SpscQueue, TwoThreadsPreserveOrder) {
  constexpr std::uint64_t kCount = 2'000'000;
  SpscQueue<std::uint64_t> q(1024);
  std::thread producer([&] {
    for (std::uint64_t i = 0; i < kCount; ++i)
      while (!q.try_push(i)) {
      }
  });
  std::uint64_t expected = 0;
  while (expected < kCount) {
    if (auto* v = q.front()) {
      ASSERT_EQ(*v, expected);
      q.pop();
      ++expected;
    }
  }
  producer.join();
}
