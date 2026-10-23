#include <gtest/gtest.h>
#include "engine_helpers.h"

using namespace mx;
using namespace mx::test;

TEST(Regression, RestsLimit) {
  MatchingEngine me(1);
  auto id = submit(me, limit(1, Side::Buy, 10000, 10));
  ASSERT_NE(me.find(id), nullptr);
  EXPECT_EQ(me.book(0).best_price(Side::Buy), 10000);
  ASSERT_EQ(me.md_events().size(), 1u);
  EXPECT_EQ(me.md_events()[0].type, MdType::Add);
}

TEST(Regression, EqualPriceCrosses) {
  MatchingEngine me(1);
  submit(me, limit(1, Side::Sell, 10000, 10));
  me.process(limit(2, Side::Buy, 10000, 5));
  EXPECT_EQ(of_type(me, ExecType::Fill).size(), 2u);
}
