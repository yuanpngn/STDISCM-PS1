
# Variant 4 — Linear Search with Per-Number Parallel Divisibility, Print After Join

This variant iterates through numbers sequentially and uses **x** threads to parallelize the divisibility testing for each individual number.

**Config file format:**
```
threads=4
limit=100000
```

- `threads` → **x** (number of divisibility-test threads per number).
- `limit` → **y** (search primes in [2, y]).

## Behavior

- Same as Variant 3, **but** primes are collected and printed **after** scanning all numbers.
- Lets you compare join/aggregation cost vs. immediate printing.

## Build & Run

### Using Make
```bash
make
./run
```

### Manual Compilation

**Linux/macOS with g++:**
```bash
g++ -std=c++17 -O2 -pthread -o run main.cpp
./run
```

**macOS with clang++:**
```bash
clang++ -std=c++17 -O2 -o run main.cpp
./run
```
*Note: `-pthread` flag is optional on macOS with clang++*

**Windows (MSYS2/MinGW):**
```bash
g++ -std=c++17 -O2 -pthread -o run.exe main.cpp
./run.exe
```
