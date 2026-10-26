# mini-exchange

A C++20 matching engine with FIX order entry and sequenced market data.

## Current implementation

- SPSC queue interface (mutex-backed)
- Chunked order and price-level storage
- Sorted price levels and FIFO order links
- Limit-order matching and execution reports
- Cancellation with ownership checks
- Market and IOC orders

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```
