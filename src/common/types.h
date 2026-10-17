#pragma once

#include <cstdint>
#include <limits>
#include <string_view>

namespace mx {

using Price = std::int64_t;
using Qty = std::uint32_t;
using OrderId = std::uint64_t;
using SymbolId = std::uint16_t;
using SessionId = std::uint32_t;
using SeqNum = std::uint64_t;

inline constexpr Price kPriceScale = 100;
inline constexpr Price kNoPrice = std::numeric_limits<Price>::min();
inline constexpr OrderId kNoOrder = 0;

enum class Side : std::uint8_t { Buy = 1, Sell = 2 };

enum class OrdType : std::uint8_t {
  Market = 1,
  Limit = 2,
  Stop = 3,
  StopLimit = 4,
};

enum class TimeInForce : std::uint8_t {
  Day = 0,
  Gtc = 1,
  Ioc = 3,
  Fok = 4,
};

constexpr Side opposite(Side s) {
  return s == Side::Buy ? Side::Sell : Side::Buy;
}

constexpr std::string_view to_string(Side s) {
  return s == Side::Buy ? "BUY" : "SELL";
}

}
