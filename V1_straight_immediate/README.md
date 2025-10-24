
# Variant 1 — Straight Division, Print Immediately

This variant divides the range [2, limit] into **x** contiguous chunks and uses **x** threads to search for primes in parallel.

**Config file format:**
```
threads=4
limit=100000
```

- `threads` → **x** (number of range-partition worker threads).
- `limit` → **y** (search primes in [2, y]).

## Behavior

- Divide range [2, limit] into **x** contiguous chunks.
- Each worker thread scans its chunk and **prints primes immediately** as they are found.
- Output includes **thread index** and **timestamp** per prime.
- Demonstrates interleaved output.

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
