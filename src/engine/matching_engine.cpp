#include "engine/matching_engine.h"

#include <algorithm>
#include <cassert>

namespace mx {

namespace {

bool needs_limit_price(OrdType t) {
  return t == OrdType::Limit || t == OrdType::StopLimit;
}
bool is_stop(OrdType t) {
  return t == OrdType::Stop || t == OrdType::StopLimit;
}

bool crosses(Side aggressor, Price limit, Price level_px) {
  return aggressor == Side::Buy ? level_px <= limit : level_px >= limit;
}

}

MatchingEngine::MatchingEngine(std::size_t num_symbols) {
  books_.reserve(num_symbols);
  for (std::size_t i = 0; i < num_symbols; ++i)
    books_.push_back(std::make_unique<OrderBook>(static_cast<SymbolId>(i), level_pool_));
  orders_.reserve(1 << 20);
  execs_.reserve(1024);
  md_.reserve(1024);
}

MatchingEngine::~MatchingEngine() {
  books_.clear();
  for (auto& [id, o] : orders_) order_pool_.release(o);
}

const Order* MatchingEngine::find(OrderId id) const {
  auto it = orders_.find(id);
  return it == orders_.end() ? nullptr : it->second;
}

void MatchingEngine::process(const Request& r) {
  execs_.clear();
  md_.clear();
  switch (r.type) {
    case RequestType::New:
      on_new(r);
      break;
    case RequestType::Cancel:
      on_cancel(r);
      break;
    case RequestType::Replace:
      on_replace(r);
      break;
  }
}

void MatchingEngine::report(ExecType type, const Order& o, std::uint64_t client_ref, bool aggressor,
                            Qty last_qty, Price last_px) {
  ExecReport& e = execs_.emplace_back();
  e.type = type;
  e.side = o.side;
  e.ord_type = o.type;
  e.aggressor = aggressor;
  e.symbol = o.symbol;
  e.owner = o.owner;
  e.client_ref = client_ref;
  e.order_id = o.id;
  e.price = o.price;
  e.last_px = last_px;
  e.last_qty = last_qty;
  e.order_qty = o.qty;
  e.leaves = o.leaves;
  e.cum = o.cum;
}

void MatchingEngine::reject(ExecType type, RejectReason why, const Request& r) {
  ExecReport& e = execs_.emplace_back();
  e.type = type;
  e.reason = why;
  e.side = r.side;
  e.ord_type = r.ord_type;
  e.symbol = r.symbol;
  e.owner = r.owner;
  e.client_ref = r.client_ref;
  e.order_id = r.order_id;
  e.price = r.price;
  e.order_qty = r.qty;
}

void MatchingEngine::destroy(Order* o) {
  orders_.erase(o->id);
  order_pool_.release(o);
}

void MatchingEngine::on_new(const Request& r) {
  if (!(r.ord_type == OrdType::Limit || r.ord_type == OrdType::Market))
    return reject(ExecType::Rejected, RejectReason::UnsupportedChange, r);
  if (r.tif == TimeInForce::Fok) return reject(ExecType::Rejected, RejectReason::UnsupportedChange, r);
  if (r.symbol >= books_.size()) return reject(ExecType::Rejected, RejectReason::UnknownSymbol, r);
  if (r.qty == 0 || r.qty > kMaxQty) return reject(ExecType::Rejected, RejectReason::InvalidQty, r);
  if (needs_limit_price(r.ord_type) && r.price <= 0)
    return reject(ExecType::Rejected, RejectReason::InvalidPrice, r);
  if (is_stop(r.ord_type) && r.stop_price <= 0)
    return reject(ExecType::Rejected, RejectReason::InvalidPrice, r);

  Order* o = order_pool_.allocate();
  o->id = next_order_id_++;
  o->owner = r.owner;
  o->symbol = r.symbol;
  o->side = r.side;
  o->type = r.ord_type;
  o->tif = r.tif;
  o->price = needs_limit_price(r.ord_type) ? r.price : 0;
  o->stop_price = is_stop(r.ord_type) ? r.stop_price : 0;
  o->qty = o->leaves = r.qty;
  orders_.emplace(o->id, o);

  report(ExecType::New, *o, r.client_ref);

  OrderBook& book = *books_[r.symbol];
  execute(o, r.client_ref);
  run_stops(book);
}

void MatchingEngine::execute(Order* o, std::uint64_t client_ref) {
  OrderBook& book = *books_[o->symbol];
  const Side resting = opposite(o->side);
  const bool is_market = o->type == OrdType::Market;

  Price last_px = kNoPrice;
  while (o->leaves > 0) {
    PriceLevel* lvl = book.best(resting);
    if (!lvl || (!is_market && !crosses(o->side, o->price, lvl->price))) break;

    Order* passive = lvl->head;
    const Qty q = std::min(o->leaves, passive->leaves);
    const Price px = lvl->price;

    o->leaves -= q;
    o->cum += q;
    book.reduce(passive, q);
    passive->cum += q;

    md_.push_back(MdEvent{MdType::Trade, o->side, o->symbol, passive->id, px, q});
    report(ExecType::Fill, *o, client_ref, /*aggressor=*/true, q, px);
    report(ExecType::Fill, *passive, 0, /*aggressor=*/false, q, px);

    if (passive->leaves == 0) {
      book.remove(passive);
      destroy(passive);
    }
    last_px = px;
  }
  if (last_px != kNoPrice) book.set_last_trade_price(last_px);

  if (o->leaves == 0) {
    destroy(o);
    return;
  }
  if (is_market || o->tif == TimeInForce::Ioc || o->tif == TimeInForce::Fok) {
    o->leaves = 0;
    report(ExecType::Expired, *o, client_ref);
    destroy(o);
    return;
  }
  book.insert(o);
  md_.push_back(MdEvent{MdType::Add, o->side, o->symbol, o->id, o->price, o->leaves});
}

void MatchingEngine::run_stops(OrderBook&) {}

void MatchingEngine::on_cancel(const Request& r) {
  auto it = orders_.find(r.order_id);
  if (it == orders_.end()) return reject(ExecType::CancelRejected, RejectReason::UnknownOrder, r);
  Order* o = it->second;
  if (o->owner != r.owner) return reject(ExecType::CancelRejected, RejectReason::NotOwner, r);

  OrderBook& book = *books_[o->symbol];
  if (o->is_stop_pending) {
    book.remove_stop(o);
  } else {
    md_.push_back(MdEvent{MdType::Delete, o->side, o->symbol, o->id, o->price, 0});
    book.remove(o);
  }
  o->leaves = 0;
  report(ExecType::Cancelled, *o, r.client_ref);
  destroy(o);
}

void MatchingEngine::on_replace(const Request& r) {
  reject(ExecType::ReplaceRejected, RejectReason::UnsupportedChange, r);
}

}
