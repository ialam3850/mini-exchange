#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "common/object_pool.h"
#include "engine/events.h"
#include "engine/order_book.h"

namespace mx {

class MatchingEngine {
 public:
  static constexpr Qty kMaxQty = 1'000'000'000;

  explicit MatchingEngine(std::size_t num_symbols);
  ~MatchingEngine();

  MatchingEngine(const MatchingEngine&) = delete;
  MatchingEngine& operator=(const MatchingEngine&) = delete;

  void process(const Request& r);

  const std::vector<ExecReport>& execs() const { return execs_; }
  const std::vector<MdEvent>& md_events() const { return md_; }

  const OrderBook& book(SymbolId s) const { return *books_[s]; }
  std::size_t num_symbols() const { return books_.size(); }
  const Order* find(OrderId id) const;
  std::size_t open_orders() const { return orders_.size(); }

 private:
  void on_new(const Request& r);
  void on_cancel(const Request& r);
  void on_replace(const Request& r);

  void execute(Order* o, std::uint64_t client_ref);

  void run_stops(OrderBook& book);

  void destroy(Order* o);
  void report(ExecType type, const Order& o, std::uint64_t client_ref, bool aggressor = false,
              Qty last_qty = 0, Price last_px = 0);
  void reject(ExecType type, RejectReason why, const Request& r);

  ObjectPool<Order> order_pool_;
  ObjectPool<PriceLevel> level_pool_;
  std::vector<std::unique_ptr<OrderBook>> books_;
  std::unordered_map<OrderId, Order*> orders_;
  OrderId next_order_id_ = 1;

  std::vector<ExecReport> execs_;
  std::vector<MdEvent> md_;
};

}
