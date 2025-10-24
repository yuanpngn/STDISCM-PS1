/**
 * @file main.cpp
 * @brief Multi-threaded prime number finder using trial division
 * 
 * This program finds all prime numbers up to a specified limit using parallel
 * computation. It divides the search range among multiple threads and merges
 * the results in sorted order using a priority queue.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>
using namespace std;

/**
 * @struct Config
 * @brief Configuration parameters for the prime finder
 */
struct Config {
    int threads = 4;           ///< Number of worker threads to spawn (default: 4)
    long long limit = 100000;  ///< Upper limit for prime search, inclusive (default: 100000)
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
 * @brief Main entry point for the multi-threaded prime finder
 * 
 * Algorithm:
 * 1. Load configuration (thread count and limit)
 * 2. Divide the range [2, limit] among worker threads
 * 3. Each thread finds primes in its assigned range
 * 4. Merge results from all threads in sorted order using a priority queue
 * 5. Output results with timing information
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

    // Storage for results from each thread
    vector<vector<long long>> buckets(T);
    vector<thread> threads;
    threads.reserve(T);

    /**
     * @brief Worker lambda function for each thread
     * @param idx Thread index (used to identify which bucket to store results)
     * @param a Start of the range to search (inclusive)
     * @param b End of the range to search (inclusive)
     * 
     * Each worker tests numbers in its assigned range and stores primes in its bucket.
     */
    auto worker = [&](int idx, long long a, long long b) {
        auto& out = buckets[idx];
        out.reserve((size_t)((b >= a) ? ((b - a + 1) / 10 + 1) : 0)); // Rough estimate for prime density
        for (long long n = a; n <= b; ++n) {
            if (is_prime_trial(n)) out.push_back(n);
        }
    };

    // Spawn worker threads, distributing the range as evenly as possible
    long long start = nmin;
    int spawned = 0;
    for (int i = 0; i < T; ++i) {
        long long len = chunk + (i < rem ? 1 : 0);
        if (len <= 0) break;
        long long a = start;
        long long b = a + len - 1;
        start = b + 1;
        threads.emplace_back(worker, i, a, b);
        ++spawned;
    }
    // Wait for all threads to complete
    for (auto& th : threads) th.join();

    // Merge results using a min-heap priority queue
    // Node represents a position in a bucket: value, bucket index, position in bucket
    struct Node { long long v; int bi; size_t pos; };
    struct Cmp { bool operator()(const Node& a, const Node& b) const { return a.v > b.v; } };
    priority_queue<Node, vector<Node>, Cmp> pq;

    // Initialize the priority queue with the first element from each non-empty bucket
    for (int i = 0; i < spawned; ++i) {
        if (!buckets[i].empty()) pq.push(Node{buckets[i][0], i, 0});
    }

    // Merge all primes in sorted order, tracking which thread found each prime
    vector<pair<long long,int>> merged;
    merged.reserve((size_t)(nmax / log(max(3LL, nmax)))); // Rough estimate using prime number theorem
    while (!pq.empty()) {
        auto cur = pq.top(); pq.pop();
        merged.emplace_back(cur.v, cur.bi);
        size_t next = cur.pos + 1;
        if (next < buckets[cur.bi].size()) {
            pq.push(Node{buckets[cur.bi][next], cur.bi, next});
        }
    }

    // Output results
    cout << "[RESULTS] total=" << merged.size() << "\n";
    for (auto& p : merged) {
        cout << "[PRIME] n=" << p.first << " found_by_thread=" << p.second << "\n";
    }
    cerr << "[SUMMARY] threads_spawned=" << spawned << "\n";
    for (int i = 0; i < spawned; ++i) {
        cerr << "[SUMMARY] thread=" << i << " primes=" << buckets[i].size() << "\n";
    }

    cout << "[END] " << now_str() << "\n";
    return 0;
}