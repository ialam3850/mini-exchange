#pragma once

#include <algorithm>
#include <vector>

#include "engine/matching_engine.h"

namespace mx::test {

inline Request limit(SessionId owner, Side side, Price px, Qty qty, TimeInForce tif = TimeInForce::Day,
                     SymbolId sym = 0) {
  Request r;
  r.type = RequestType::New;
  r.owner = owner;
  r.symbol = sym;
  r.side = side;
  r.ord_type = OrdType::Limit;
  r.tif = tif;
  r.price = px;
  r.qty = qty;
  return r;
}

inline Request market(SessionId owner, Side side, Qty qty, SymbolId sym = 0) {
  Request r = limit(owner, side, 0, qty, TimeInForce::Day, sym);
  r.ord_type = OrdType::Market;
  return r;
}

inline Request stop(SessionId owner, Side side, Price trigger, Qty qty, Price limit_px = 0) {
  Request r = limit(owner, side, limit_px, qty);
  r.ord_type = limit_px > 0 ? OrdType::StopLimit : OrdType::Stop;
  r.stop_price = trigger;
  return r;
}

inline Request cancel(SessionId owner, OrderId id) {
  Request r;
  r.type = RequestType::Cancel;
  r.owner = owner;
  r.order_id = id;
  return r;
}

inline Request replace(SessionId owner, OrderId id, Side side, Price px, Qty total_qty) {
  Request r;
  r.type = RequestType::Replace;
  r.owner = owner;
  r.order_id = id;
  r.side = side;
  r.price = px;
  r.qty = total_qty;
  return r;
}

inline OrderId submit(MatchingEngine& me, const Request& r) {
  me.process(r);
  for (const auto& e : me.execs())
    if (e.type == ExecType::New) return e.order_id;
  return kNoOrder;
}

inline std::vector<ExecReport> of_type(const MatchingEngine& me, ExecType t) {
  std::vector<ExecReport> out;
  std::copy_if(me.execs().begin(), me.execs().end(), std::back_inserter(out),
               [t](const ExecReport& e) { return e.type == t; });
  return out;
}

inline std::vector<OrderId> queue_at_best(const MatchingEngine& me, Side side, SymbolId sym = 0) {
  std::vector<OrderId> ids;
  const PriceLevel* lvl = me.book(sym).best(side);
  for (const Order* o = lvl ? lvl->head : nullptr; o; o = o->next) ids.push_back(o->id);
  return ids;
}

}
