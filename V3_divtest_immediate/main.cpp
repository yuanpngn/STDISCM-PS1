/**
 * @file main.cpp
 * @brief Multi-threaded prime number finder with parallelized divisibility testing
 * 
 * This program finds all prime numbers up to a specified limit using a unique approach:
 * instead of dividing the NUMBER RANGE among threads, it parallelizes the DIVISIBILITY 
 * TESTING for each individual number. Multiple threads cooperatively test divisors for 
 * each candidate number.
 * 
 * Key characteristics:
 * - Sequential number iteration (single-threaded)
 * - Parallel divisibility testing (multi-threaded per number)
 * - Early termination when any thread finds a divisor
 * - Immediate output as each prime is confirmed
 * - Uses atomic operations for thread coordination
 * 
 * Trade-offs:
 * + Better for testing very large individual numbers
 * - Higher thread creation overhead (threads spawned per number)
 * - Sequential bottleneck in main loop
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
    int threads = 4;           ///< Number of threads for parallel divisibility testing (default: 4)
    long long limit = 100000;  ///< Upper limit for prime search, inclusive (default: 100000)
};

/**
 * @brief Get current system time as a formatted string with millisecond precision
 * @return String in format "YYYY-MM-DD HH:MM:SS.mmm"
 * 
 * Uses system clock to get current time and formats it with millisecond precision.
 * Platform-specific code handles differences between Windows and POSIX systems.
 * Used for timestamping the start/end of execution and each prime discovery.
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
    // Lambda to trim whitespace from both ends of a string
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
 */
bool is_prime_parallel(long long n, int T) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    if (n % 3 == 0) return n == 3;
    long long hi = (long long)floor(sqrt((long double)n));
    if (hi < 5) return true;  // No more divisors to check

    // Shared atomic flag: set to true if any thread finds a divisor
    atomic<bool> composite{false};
    vector<thread> workers;
    workers.reserve((size_t)T);

    /**
     * @brief Worker lambda for parallel divisibility testing
     * @param idx Thread index (0 to T-1)
     * 
     * Each worker tests a strided sequence of potential divisors:
     * - Thread 0 tests: 5, 5+2T, 5+4T, ...
     * - Thread 1 tests: 7, 7+2T, 7+4T, ...
     * - Thread i tests: 5+2i, 5+2i+2T, 5+2i+4T, ...
     * 
     * The stride of 2T ensures:
     * - All threads test only odd numbers (even divisors already handled)
     * - Work is distributed evenly across threads
     * - No overlap between threads
     */
    auto worker = [&](int idx) {
        // Starting divisor for this thread
        long long start = 5 + 2LL * idx;
        for (long long d = start; d <= hi && !composite.load(memory_order_relaxed); d += 2LL * T) {
            // Skip multiples of 3 (already tested n % 3)
            if (d % 3 == 0) continue;
            if (n % d == 0) { composite.store(true, memory_order_relaxed); break; }
        }
    };

    // Spawn all worker threads
    for (int i = 0; i < T; ++i) workers.emplace_back(worker, i);
    // Wait for all workers to complete
    for (auto& th : workers) th.join();
    return !composite.load(memory_order_relaxed);
}

/**
 * @brief Main entry point for the parallel divisibility testing prime finder
 * 
 * Algorithm:
 * 1. Load configuration (thread count and limit)
 * 2. Iterate sequentially through numbers from 2 to limit
 * 3. For each number, spawn T threads to test divisibility in parallel
 * 4. If prime, immediately output with timestamp and metadata
 * 5. Continue until all numbers are tested
 * 
 * Key characteristics:
 * - Sequential outer loop (single-threaded number iteration)
 * - Parallel inner testing (multi-threaded divisibility checks)
 * - High thread creation overhead (T threads spawned per number tested)
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

    // Sequential iteration through all candidate numbers
    for (long long n = 2; n <= nmax; ++n) {
        // Parallel divisibility testing for this specific number
        if (is_prime_parallel(n, T)) {
            // Immediately output when prime is confirmed
            cout << "[PRIME] n=" << n
                 << " tid=" << this_thread::get_id()
                 << " div_threads=" << T
                 << " ts=" << now_str() << "\n";
        }
    }

    cout << "[END] " << now_str() << "\n";
    return 0;
}