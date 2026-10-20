#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "common/object_pool.h"
#include "common/types.h"

namespace mx {

struct PriceLevel;

struct Order {
  OrderId id = kNoOrder;
  SessionId owner = 0;
  SymbolId symbol = 0;
  Side side = Side::Buy;
  OrdType type = OrdType::Limit;
  TimeInForce tif = TimeInForce::Day;
  Price price = 0;
  Price stop_price = 0;
  Qty qty = 0;
  Qty leaves = 0;
  Qty cum = 0;
  bool is_stop_pending = false;

  Order* prev = nullptr;
  Order* next = nullptr;
  PriceLevel* level = nullptr;
};

struct PriceLevel {
  Price price = 0;
  std::uint64_t total_qty = 0;
  std::uint32_t count = 0;
  Order* head = nullptr;
  Order* tail = nullptr;
};

class OrderBook {
 public:
  OrderBook(SymbolId symbol, ObjectPool<PriceLevel>& level_pool);
  ~OrderBook();

  OrderBook(const OrderBook&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

  SymbolId symbol() const { return symbol_; }

  void insert(Order* o);

  void remove(Order* o);

  void reduce(Order* o, Qty by);

  PriceLevel* best(Side side) const {
    const auto& v = levels(side);
    return v.empty() ? nullptr : v.back();
  }
  Price best_price(Side side) const {
    auto* l = best(side);
    return l ? l->price : kNoPrice;
  }

  std::uint64_t available(Side resting_side, Price limit, bool is_market, std::uint64_t needed) const;

  const std::vector<PriceLevel*>& levels(Side side) const { return side == Side::Buy ? bids_ : asks_; }
  std::size_t level_count(Side side) const { return levels(side).size(); }

  void add_stop(Order* o);
  void remove_stop(Order* o);

  Order* pop_triggered_stop(Price last_trade);
  std::size_t stop_count() const { return buy_stops_.size() + sell_stops_.size(); }

  Price last_trade_price() const { return last_trade_; }
  void set_last_trade_price(Price p) { last_trade_ = p; }

 private:
  std::vector<PriceLevel*>& levels(Side side) { return side == Side::Buy ? bids_ : asks_; }
  PriceLevel* find_or_create_level(Side side, Price price);

  SymbolId symbol_;
  ObjectPool<PriceLevel>& level_pool_;
  std::vector<PriceLevel*> bids_;
  std::vector<PriceLevel*> asks_;

  std::multimap<Price, Order*> buy_stops_;
  std::multimap<Price, Order*, std::greater<Price>> sell_stops_;
  Price last_trade_ = kNoPrice;
};

}
