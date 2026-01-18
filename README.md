# High-Performance Limit Order Book

A high-performance, single-threaded limit order book matching engine optimized for low-latency trading systems.

## Matching Rules

The order book implements **price-time priority** matching:

1. **Price Priority**: Orders match first by price
   - Buy orders match against the **best ask** (lowest sell price)
   - Sell orders match against the **best bid** (highest buy price)
   - A buy order matches if its limit price `>=` best ask price
   - A sell order matches if its limit price `<=` best bid price

2. **Time Priority (FIFO)**: Within the same price level, orders match in **first-in-first-out** order
   - The oldest resting order at a price level matches first
   - Implemented using intrusive doubly-linked lists for O(1) operations

3. **Partial Fills**: Orders can be partially filled
   - An incoming order may match against multiple resting orders
   - A resting order may be partially filled by multiple incoming orders
   - Each fill creates a separate trade record

4. **Trade Price**: The trade price is always the **resting order's limit price** (maker-taker pricing)

5. **Matching Process**:
   - Incoming orders attempt to match immediately upon placement
   - Matching continues until the incoming order is fully filled or no more matches are available
   - Unfilled portions become resting orders in the book
   - Fulfilled orders are immediately reclaimed from memory

## Determinism Guarantees

The order book provides **deterministic execution**:

- **Single-threaded**: No race conditions or non-deterministic thread scheduling
- **Deterministic data structures**: 
  - `std::unordered_map` with deterministic hash function
  - `std::set` for sorted price tracking
  - FIFO ordering within price levels ensures consistent matching order
- **No randomness**: All operations are deterministic based on input order sequence
- **Reproducible results**: Given the same sequence of order operations, the book state and trades are identical

This determinism is critical for:
- Backtesting and simulation accuracy
- Regulatory compliance and audit trails
- Debugging and testing reproducibility

## Why Single-Threaded

This implementation is **intentionally single-threaded** for the following reasons:

1. **Performance**: Eliminates locking overhead, atomic operations, and cache coherency penalties
   - Zero synchronization costs in the hot path
   - Predictable execution time without lock contention

2. **Low Latency**: Single-threaded execution minimizes latency variance
   - No context switching overhead
   - Better CPU cache locality (data stays hot in L1/L2 cache)
   - Predictable branch prediction and instruction pipeline behavior

3. **Simplicity**: No need for complex concurrency control
   - No race conditions to debug
   - Easier to reason about correctness
   - Simpler memory model

4. **Determinism**: Ensures reproducible, deterministic matching behavior
   - Critical for backtesting and regulatory compliance
   - Eliminates non-deterministic thread scheduling effects

5. **Scalability Model**: Designed for high-throughput single-threaded processing
   - Modern CPUs can process millions of orders per second on a single core
   - For multi-core scaling, use multiple order book instances (one per thread/core)
   - This "sharding" approach is common in high-frequency trading systems

## How to Run

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run Main Executable

```bash
./LOB
```

### Run Tests

```bash
./LOBTest
```

### Run Benchmark

```bash
# Run with default 10 million messages
./LOBBench

# Run with custom number of messages
./LOBBench 100000000
```
