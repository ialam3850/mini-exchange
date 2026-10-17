# mini-exchange

A C++20 matching engine with FIX order entry and sequenced market data.

## Current implementation

Build scaffold; implementation follows.

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```
