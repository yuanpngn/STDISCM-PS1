
# Threaded Prime Search — Four Variants

This package contains four C++17 implementations to explore threading, printing, and task division schemes for prime search up to **y**, with **x** threads configurable via `config.txt`.

Each variant prints a start and end timestamp. Build with any C++17 compiler (tested with `g++` on macOS/Linux and `clang++` on macOS).

**Config file format** (same for all variants):
```
threads=4
limit=100000
```

- `threads` → **x** (number of threads).
  - For Variants 1–2, this is the number of **range-partition workers**.
  - For Variants 3–4, this is the number of **divisibility-test threads per number**.
- `limit` → **y** (search primes in [2, y]).

## Build (generic)

From inside any variant folder:
```bash
g++ -std=c++17 -O2 -pthread -o run main.cpp
```

Run:
```bash
./run
```

macOS note: `-pthread` is accepted by `clang++`; if you see a warning, you can omit it:
```bash
clang++ -std=c++17 -O2 -o run main.cpp
```

Windows (MSYS2 / WSL) example:
```bash
g++ -std=c++17 -O2 -o run.exe main.cpp
./run.exe
```

---

## Variant 1 — Straight Division, Print Immediately

- Divide range [2, limit] into **x** contiguous chunks.
- Each worker thread scans its chunk and **prints primes immediately** as they are found.
- Output includes **thread index** and **timestamp** per prime.
- Demonstrates interleaved output.

### Run example
```bash
make
./run
```
