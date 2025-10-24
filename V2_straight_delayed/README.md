
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

## Variant 2 — Straight Division, Print After Join

- Same contiguous chunk partitioning as Variant 1.
- Each thread collects primes locally; printing happens **only after all threads finish**.
- Output is consolidated (sorted), with thread index attribution.
- Demonstrates the effect of join-and-print later.

### Run example
```bash
make
./run
```
