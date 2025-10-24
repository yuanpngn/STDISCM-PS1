/**
 * @file main.cpp
 * @brief Multi-threaded prime number finder with parallelized divisibility testing and delayed output
 * 
 * This program finds all prime numbers up to a specified limit by parallelizing the
 * divisibility testing for each individual number, then collecting and sorting results
 * before outputting them at the end.
 * 
 * Key characteristics:
 * - Sequential number iteration (single-threaded outer loop)
 * - Parallel divisibility testing (multi-threaded per number)
 * - Delayed batch output (collects all primes, sorts, then prints)
 * - Early termination when any thread finds a divisor
 * - Uses atomic operations for thread coordination
 * 
 * Comparison with V3 (divtest_immediate):
 * - V3: Prints primes immediately as found (unordered output)
 * - V4: Collects primes, sorts, and prints in order (this variant)
 * 
 * Trade-offs:
 * + Sorted output for better readability
 * + Batch output avoids I/O interleaving
 * - Higher memory usage (stores all primes)
 * - Delayed feedback (no output until completion)
 * - High thread creation overhead (threads spawned per number)
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace std;

/**
 * @struct Config
 * @brief Configuration parameters for the prime finder
 */
struct Config {
    int threads = 4;          
    long long limit = 100000; 
};

/**
 * @brief Get current system time as a formatted string with millisecond precision
 * @return String in format "YYYY-MM-DD HH:MM:SS.mmm"
 * 
 * Uses system clock to get current time and formats it with millisecond precision.
 * Platform-specific code handles differences between Windows and POSIX systems.
 */
inline string now_str() {
    using namespace std::chrono;
    auto now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
    tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    char out[80];
    snprintf(out, sizeof(out), "%s.%03lld", buf, (long long)ms.count());
    return string(out);
}

/**
 * @brief Load configuration from a text file
 * @param path Path to the configuration file (default: "config.txt")
 * @return Config object with loaded or default values
 * 
 * Reads a simple key=value format configuration file.
 * Lines starting with '#' are treated as comments.
 * If file cannot be opened or values are invalid, defaults are used.
 * Validates thread count and limit values, setting sensible minimums.
 */
Config load_config(const string& path = "config.txt") {
    Config c;
    ifstream in(path);
    if (!in) {
        cerr << "[WARN] Could not open " << path << ", using defaults.\n";
        return c;
    }
    string line;
    auto trim = [](string s) {
        auto l = s.find_first_not_of(" \t\r\n");
        auto r = s.find_last_not_of(" \t\r\n");
        if (l == string::npos) return string();
        return s.substr(l, r - l + 1);
    };
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == string::npos) continue;
        string k = trim(line.substr(0, eq));
        string v = trim(line.substr(eq + 1));
        if (k == "threads") c.threads = stoi(v);
        else if (k == "limit") c.limit = stoll(v);
    }
    if (c.threads <= 0) c.threads = max(1u, thread::hardware_concurrency());
    if (c.limit < 2) c.limit = 2;
    return c;
}

/**
 * @brief Test if a number is prime using parallel divisibility testing
 * @param n The number to test for primality
 * @param T Number of threads to use for divisibility testing
 * @return true if n is prime, false otherwise
 * 
 * This function uses a parallel approach to test primality:
 * 1. Handles special cases (< 2, divisible by 2 or 3)
 * 2. Spawns T worker threads to test divisibility in parallel
 * 3. Each thread tests a subset of potential divisors from 5 to √n
 * 4. Threads are interleaved: thread i tests divisors 5+2i, 5+2i+2T, 5+2i+4T, ...
 * 5. Skips multiples of 3 (optimization since already tested)
 * 6. Uses atomic flag for early termination when any divisor is found
 * 
 * Thread coordination:
 * - atomic<bool> composite: Shared flag indicating if a divisor was found
 * - memory_order_relaxed: Used for performance (strict ordering not required)
 * - Early exit: Threads check the flag and stop if another thread found a divisor
 * 
 * Performance notes:
 * - Best for testing very large individual numbers where √n is large
 * - Thread creation overhead is significant for small numbers
 * - Early termination reduces wasted work for composite numbers
 */
bool is_prime_parallel(long long n, int T) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    if (n % 3 == 0) return n == 3;
    long long hi = (long long)floor(sqrt((long double)n));
    if (hi < 5) return true;

    atomic<bool> composite{false};
    vector<thread> workers;
    workers.reserve((size_t)T);

    auto worker = [&](int idx) {
        long long start = 5 + 2LL * idx;
        for (long long d = start; d <= hi && !composite.load(memory_order_relaxed); d += 2LL * T) {
            if (d % 3 == 0) continue;
            if (n % d == 0) { composite.store(true, memory_order_relaxed); break; }
        }
    };

    for (int i = 0; i < T; ++i) workers.emplace_back(worker, i);
    for (auto& th : workers) th.join();
    return !composite.load(memory_order_relaxed);
}

/**
 * @brief Main entry point for the parallel divisibility testing prime finder with delayed output
 * 
 * Algorithm:
 * 1. Load configuration (thread count and limit)
 * 2. Iterate sequentially through numbers from 2 to limit
 * 3. For each number, spawn T threads to test divisibility in parallel
 * 4. Collect all primes in a vector
 * 5. Sort the collected primes (ensures ordered output)
 * 6. Output all primes in a batch at the end
 * 
 * Key characteristics:
 * - Sequential outer loop (single-threaded number iteration)
 * - Parallel inner testing (multi-threaded divisibility checks)
 * - Delayed batch output (no printing until all primes found)
 * - Sorted results for readability
 * - Memory pre-allocation using prime number theorem estimate (n/ln(n))
 * 
 * Performance considerations:
 * - High thread creation overhead (T threads spawned per number tested)
 * - Memory usage grows with number of primes found
 * - Sorting overhead at the end (though typically small)
 * - Best for scenarios where individual numbers are very large
 * 
 * @return 0 on successful completion
 */
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Config cfg = load_config();
    cout << "[START] " << now_str() << "\n";

    const long long nmax = cfg.limit;
    const int T = max(1, cfg.threads);

    vector<long long> primes;
    // crude upper bound to reduce realloc (n/log n)
    if (nmax >= 3) {
        primes.reserve((size_t)(nmax / log((long double)max(3LL, nmax))));
    }

    for (long long n = 2; n <= nmax; ++n) {
        if (is_prime_parallel(n, T)) primes.push_back(n);
    }

    sort(primes.begin(), primes.end());
    cout << "[RESULTS] total=" << primes.size() << "\n";
    for (auto n : primes) {
        cout << "[PRIME] n=" << n << "\n";
    }

    cout << "[END] " << now_str() << "\n";
    return 0;
}