# High-Performance Limit Order Book

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
