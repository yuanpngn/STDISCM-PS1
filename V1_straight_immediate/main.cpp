/**
 * @file main.cpp
 * @brief Multi-threaded prime number finder with immediate output
 * 
 * This program finds all prime numbers up to a specified limit using parallel
 * computation. Unlike the delayed output variant, this version prints each prime
 * immediately as it is found, using a mutex to serialize output from multiple threads.
 * 
 * Key characteristics:
 * - Immediate output: Primes are printed as soon as they are found
 * - Thread-safe printing: Uses mutex to prevent interleaved output
 * - Includes timestamps and thread IDs for each prime found
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>
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
 * @brief Test if a number is prime using trial division
 * @param n The number to test for primality
 * @return true if n is prime, false otherwise
 * 
 * Uses optimized trial division with special cases for 2 and 3,
 * then checks divisibility by numbers of form 6k±1 up to √n.
 * This optimization is based on the fact that all primes > 3 are of form 6k±1.
 */
inline bool is_prime_trial(long long n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    if (n % 3 == 0) return n == 3;
    for (long long d = 5; d * d <= n; d += 6) {
        if (n % d == 0 || n % (d + 2) == 0) return false;
    }
    return true;
}

/**
 * @brief Main entry point for the multi-threaded prime finder with immediate output
 * 
 * Algorithm:
 * 1. Load configuration (thread count and limit)
 * 2. Divide the range [2, limit] among worker threads
 * 3. Each thread finds primes in its assigned range and immediately prints them
 * 4. Uses mutex to ensure thread-safe printing without interleaved output
 * 5. Waits for all threads to complete
 * 
 * Note: Output order is non-deterministic due to race conditions between threads.
 * Primes are printed as discovered, not in numerical order.
 * 
 * @return 0 on successful completion
 */
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Config cfg = load_config();
    cout << "[START] " << now_str() << "\n";

    // Define the search range [nmin, nmax]
    const long long nmin = 2;
    const long long nmax = cfg.limit;
    const int T = max(1, cfg.threads);

    // Calculate how to divide the range among threads
    const long long span = (nmax >= nmin) ? (nmax - nmin + 1) : 0;
    const long long chunk = (T > 0) ? (span / T) : span;
    const long long rem = (T > 0) ? (span % T) : 0;

    // Mutex for thread-safe printing
    mutex print_mtx;
    vector<thread> threads;
    threads.reserve(T);

    /**
     * @brief Worker lambda function for each thread
     * @param idx Thread index (worker ID for identification)
     * @param a Start of the range to search (inclusive)
     * @param b End of the range to search (inclusive)
     * 
     * Each worker tests numbers in its assigned range. When a prime is found,
     * it acquires the print mutex and immediately outputs the prime with metadata:
     * - The prime number itself
     * - Worker ID
     * - Thread ID
     * - Timestamp of discovery
     */
    auto worker = [&](int idx, long long a, long long b) {
        for (long long n = a; n <= b; ++n) {
            if (is_prime_trial(n)) {
                lock_guard<mutex> lk(print_mtx);
                cout << "[PRIME] n=" << n
                     << " worker=" << idx
                     << " tid=" << this_thread::get_id()
                     << " ts=" << now_str() << "\n";
            }
        }
    };


    long long start = nmin;
    for (int i = 0; i < T; ++i) {
        long long len = chunk + (i < rem ? 1 : 0);
        if (len <= 0) break;  
        long long a = start;
        long long b = a + len - 1;
        start = b + 1;
        threads.emplace_back(worker, i, a, b);
    }

    for (auto& th : threads) th.join();

    cout << "[END] " << now_str() << "\n";
    return 0;
}