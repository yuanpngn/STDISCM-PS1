
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

## Variant 3 — Linear Search; Per-Number Divisibility Parallelized; Print Immediately

- Iterate `n` from 2..limit **sequentially**.
- For each `n`, spawn **x threads** that split the divisor range `2..floor(sqrt(n))` and test in parallel.
- If `n` is prime, print **immediately** with timestamp.
- This highlights overhead from creating/joining threads for **every candidate** and potential speedups for very large `n`.

### Run example
```bash
make
./run
```
