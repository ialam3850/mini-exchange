#include "engine/order_book.h"

#include <algorithm>
#include <cassert>

namespace mx {

namespace {

inline bool worse(Side side, Price a, Price b) {
  return side == Side::Buy ? a < b : a > b;
}

}

OrderBook::OrderBook(SymbolId symbol, ObjectPool<PriceLevel>& level_pool)
    : symbol_(symbol), level_pool_(level_pool) {
  bids_.reserve(1024);
  asks_.reserve(1024);
}

OrderBook::~OrderBook() {

  for (auto* l : bids_) level_pool_.release(l);
  for (auto* l : asks_) level_pool_.release(l);
}

PriceLevel* OrderBook::find_or_create_level(Side side, Price price) {
  auto& v = levels(side);

  if (!v.empty() && v.back()->price == price) return v.back();

  auto it = std::lower_bound(v.begin(), v.end(), price,
                             [side](const PriceLevel* l, Price p) { return worse(side, l->price, p); });
  if (it != v.end() && (*it)->price == price) return *it;

  PriceLevel* lvl = level_pool_.allocate();
  lvl->price = price;
  v.insert(it, lvl);
  return lvl;
}

void OrderBook::insert(Order* o) {
  PriceLevel* lvl = find_or_create_level(o->side, o->price);
  o->level = lvl;
  o->next = nullptr;
  o->prev = lvl->tail;
  if (lvl->tail)
    lvl->tail->next = o;
  else
    lvl->head = o;
  lvl->tail = o;
  lvl->total_qty += o->leaves;
  ++lvl->count;
}

void OrderBook::remove(Order* o) {
  PriceLevel* lvl = o->level;
  assert(lvl && "order is not resting");

  if (o->prev)
    o->prev->next = o->next;
  else
    lvl->head = o->next;
  if (o->next)
    o->next->prev = o->prev;
  else
    lvl->tail = o->prev;
  lvl->total_qty -= o->leaves;
  --lvl->count;
  o->prev = o->next = nullptr;
  o->level = nullptr;

  if (lvl->count == 0) {
    auto& v = levels(o->side);
    if (v.back() == lvl) {
      v.pop_back();
    } else {
      auto it = std::lower_bound(
          v.begin(), v.end(), lvl->price,
          [side = o->side](const PriceLevel* l, Price p) { return worse(side, l->price, p); });
      assert(it != v.end() && *it == lvl);
      v.erase(it);
    }
    level_pool_.release(lvl);
  }
}

void OrderBook::reduce(Order* o, Qty by) {
  assert(o->level && by <= o->leaves);
  o->leaves -= by;
  o->level->total_qty -= by;
}

std::uint64_t OrderBook::available(Side resting_side, Price limit, bool is_market,
                                   std::uint64_t needed) const {
  const auto& v = levels(resting_side);
  std::uint64_t sum = 0;
  for (auto it = v.rbegin(); it != v.rend() && sum < needed; ++it) {
    const Price p = (*it)->price;

    const bool crosses = is_market || (resting_side == Side::Sell ? p <= limit : p >= limit);
    if (!crosses) break;
    sum += (*it)->total_qty;
  }
  return sum;
}

void OrderBook::add_stop(Order* o) {
  o->is_stop_pending = true;
  if (o->side == Side::Buy)
    buy_stops_.emplace(o->stop_price, o);
  else
    sell_stops_.emplace(o->stop_price, o);
}

void OrderBook::remove_stop(Order* o) {
  auto erase_from = [o](auto& m) {
    auto [lo, hi] = m.equal_range(o->stop_price);
    for (auto it = lo; it != hi; ++it) {
      if (it->second == o) {
        m.erase(it);
        return true;
      }
    }
    return false;
  };
  [[maybe_unused]] bool found = o->side == Side::Buy ? erase_from(buy_stops_) : erase_from(sell_stops_);
  assert(found);
  o->is_stop_pending = false;
}

Order* OrderBook::pop_triggered_stop(Price last_trade) {
  if (last_trade == kNoPrice) return nullptr;

  if (!buy_stops_.empty() && buy_stops_.begin()->first <= last_trade) {
    Order* o = buy_stops_.begin()->second;
    buy_stops_.erase(buy_stops_.begin());
    o->is_stop_pending = false;
    return o;
  }
  if (!sell_stops_.empty() && sell_stops_.begin()->first >= last_trade) {
    Order* o = sell_stops_.begin()->second;
    sell_stops_.erase(sell_stops_.begin());
    o->is_stop_pending = false;
    return o;
  }
  return nullptr;
}

}
