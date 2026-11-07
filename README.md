# mini-exchange

A C++20 matching engine with FIX order entry and sequenced market data.

## Current implementation

- SPSC queue interface (mutex-backed)
- Chunked order and price-level storage
- Sorted price levels and FIFO order links
- Limit-order matching and execution reports
- Cancellation with ownership checks
- Market and IOC orders
- FOK availability checks

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

## Development history

Local development: **July 2024–present**, using the start date supplied by the project author.

The existing Git history begins on **October 17, 2026**.

See the [development history](docs/DEVELOPMENT_HISTORY.md) for dated milestones in the repository.
