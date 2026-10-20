#pragma once

#include <cstdint>

#include "common/types.h"

namespace mx {

enum class RequestType : std::uint8_t { New, Cancel, Replace };

struct Request {
  RequestType type = RequestType::New;
  SessionId owner = 0;

  std::uint64_t client_ref = 0;
  SymbolId symbol = 0;
  Side side = Side::Buy;
  OrdType ord_type = OrdType::Limit;
  TimeInForce tif = TimeInForce::Day;
  Price price = 0;
  Price stop_price = 0;
  Qty qty = 0;
  OrderId order_id = kNoOrder;
};

enum class ExecType : std::uint8_t {
  New,
  Fill,
  Cancelled,
  Expired,
  Replaced,
  Triggered,
  Rejected,
  CancelRejected,
  ReplaceRejected,
};

enum class RejectReason : std::uint8_t {
  None,
  UnknownSymbol,
  InvalidQty,
  InvalidPrice,
  UnknownOrder,
  NotOwner,
  TooLateToReplace,
  UnsupportedChange,
};

struct ExecReport {
  ExecType type = ExecType::New;
  RejectReason reason = RejectReason::None;
  Side side = Side::Buy;
  OrdType ord_type = OrdType::Limit;
  bool aggressor = false;
  SymbolId symbol = 0;
  SessionId owner = 0;
  std::uint64_t client_ref = 0;
  OrderId order_id = kNoOrder;
  Price price = 0;
  Price last_px = 0;
  Qty last_qty = 0;
  Qty order_qty = 0;
  Qty leaves = 0;
  Qty cum = 0;
};

enum class MdType : std::uint8_t {
  Add = 'A',
  Modify = 'U',
  Delete = 'D',
  Trade = 'T',
};

struct MdEvent {
  MdType type = MdType::Add;
  Side side = Side::Buy;
  SymbolId symbol = 0;
  OrderId order_id = kNoOrder;
  Price price = 0;
  Qty qty = 0;
};

}
