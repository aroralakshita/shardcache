//shardcache benchmark tool. build and run in Release for meaningful numbers

//load test: throughput and latency percentiles
//failover test: one client sends requests nonstop; close one server window mid-run

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <latch>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "bench/stats.h"
#include "router/router.h"

using Clock = std::chrono::steady_clock;
using shardcache::Router;

namespace {

const std::string kValue = "vvvvvvvvvvvvvvvv";

struct Options {
    std::string mode;
    std::vector<std::string> nodes;
    int threads = 4;
    int seconds = 0;  //0 = mode default: 10 s for load, 20 s for failover
    int keys = 10000;
    double get_ratio = 0.8;
    std::size_t replicas = 2;
    int interval_ms = 1000;
    int misses = 3;
};

std::vector<std::string> split_commas(const std::string& text) {
    std::vector<std::string> parts;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, ',')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

Options parse_options(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::invalid_argument("usage: shardcache_bench load|failover --nodes a:p,b:p,... [options]");
    }
    Options o;
    o.mode = argv[1];
    for (int i = 2; i + 1 < argc; i += 2) {
        std::string flag = argv[i];
        std::string value = argv[i + 1];
        if (flag == "--nodes") o.nodes = split_commas(value);
        else if (flag == "--threads") o.threads = std::stoi(value);
        else if (flag == "--seconds") o.seconds = std::stoi(value);
        else if (flag == "--keys") o.keys = std::stoi(value);
        else if (flag == "--get-ratio") o.get_ratio = std::stod(value);
        else if (flag == "--replicas") o.replicas = std::stoul(value);
        else if (flag == "--interval-ms") o.interval_ms = std::stoi(value);
        else if (flag == "--misses") o.misses = std::stoi(value);
        else throw std::invalid_argument("unknown option " + flag);
    }
    if (o.nodes.empty()) {
        throw std::invalid_argument("--nodes is required");
    }
    if (o.seconds == 0) {
        o.seconds = (o.mode == "failover") ? 20 : 10;
    }
    return o;
}

bool is_error(const std::string& reply) {
    return reply.rfind("ERROR", 0) == 0;
}

//fills the cluster with every key once so GETs measure hits
void populate(const Options& o) {
    Router router(o.nodes, o.replicas);
    for (int i = 0; i < o.keys; ++i) {
        std::string reply = router.request("SET key:" + std::to_string(i) + " " + kValue);
        if (reply != "OK") {
            throw std::runtime_error("populate failed: " + reply);
        }
    }
}

class Workload {
public:
    Workload(const Options& o, unsigned seed)
        : rng_(seed), key_(0, o.keys - 1), coin_(0.0, 1.0), get_ratio_(o.get_ratio) {}

    std::string next() {
        std::string key = "key:" + std::to_string(key_(rng_));
        return coin_(rng_) < get_ratio_ ? "GET " + key : "SET " + key + " " + kValue;
    }

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> key_;
    std::uniform_real_distribution<double> coin_;
    double get_ratio_;
};

void run_load(const Options& o) {
    populate(o);

    struct ThreadResult {
        std::vector<double> latencies_us;
        long long errors = 0;
    };
    std::vector<ThreadResult> results(static_cast<std::size_t>(o.threads));

    //synchronize the workers so they begin the measured workload together
    std::latch ready(o.threads + 1);
    std::vector<std::thread> workers;
    for (int t = 0; t < o.threads; ++t) {
        workers.emplace_back([&o, &results, &ready, t] {
            Router router(o.nodes, o.replicas);
            Workload workload(o, static_cast<unsigned>(t + 1));
            ThreadResult& result = results[static_cast<std::size_t>(t)];
            ready.arrive_and_wait();

            auto deadline = Clock::now() + std::chrono::seconds(o.seconds);
            while (Clock::now() < deadline) {
                std::string line = workload.next();
                auto start = Clock::now();
                std::string reply = router.request(line);
                auto end = Clock::now();
                if (is_error(reply)) {
                    ++result.errors;
                    continue;
                }
                result.latencies_us.push_back(
                    std::chrono::duration<double, std::micro>(end - start).count());
            }
        });
    }
    ready.arrive_and_wait();
    auto start = Clock::now();
    for (std::thread& worker : workers) {
        worker.join();
    }
    double elapsed_s = std::chrono::duration<double>(Clock::now() - start).count();

    std::vector<double> all;
    long long errors = 0;
    for (const ThreadResult& result : results) {
        all.insert(all.end(), result.latencies_us.begin(), result.latencies_us.end());
        errors += result.errors;
    }
    std::sort(all.begin(), all.end());

    std::cout << std::fixed << std::setprecision(1)
              << "nodes=" << o.nodes.size() << " replicas=" << o.replicas
              << " threads=" << o.threads << " keys=" << o.keys
              << " get_ratio=" << o.get_ratio << " duration=" << elapsed_s << "s\n"
              << "  operations: " << all.size() << " ok, " << errors << " errors\n"
              << "  throughput: " << static_cast<double>(all.size()) / elapsed_s << " ops/sec\n"
              << "  latency (us): p50=" << shardcache::percentile_of_sorted(all, 50)
              << "  p95=" << shardcache::percentile_of_sorted(all, 95)
              << "  p99=" << shardcache::percentile_of_sorted(all, 99)
              << "  max=" << (all.empty() ? 0.0 : all.back()) << "\n";
}

void run_failover(const Options& o) {
    populate(o);
    Router router(o.nodes, o.replicas);
    router.enable_failure_detection(std::chrono::milliseconds(o.interval_ms), o.misses);
    Workload workload(o, 42);

    std::cout << "Running for " << o.seconds << " s with heartbeats every " << o.interval_ms
              << " ms, " << o.misses << " misses to fail.\n"
              << ">>> Close ONE server window in the next few seconds. <<<\n";

    auto start = Clock::now();
    auto deadline = start + std::chrono::seconds(o.seconds);
    auto next_report = start + std::chrono::seconds(1);
    long long ok = 0, failed = 0, ok_this_second = 0, failed_this_second = 0;
    std::optional<Clock::time_point> first_failure, last_failure;

    while (Clock::now() < deadline) {
        std::string reply = router.request(workload.next());
        auto now = Clock::now();
        if (is_error(reply)) {
            ++failed;
            ++failed_this_second;
            if (!first_failure.has_value()) {
                first_failure = now;
            }
            last_failure = now;
        } else {
            ++ok;
            ++ok_this_second;
        }
        if (now >= next_report) {
            auto t = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            std::cout << "  t=" << std::setw(2) << t << "s  ok=" << std::setw(6) << ok_this_second
                      << "  failed=" << failed_this_second << "\n";
            ok_this_second = 0;
            failed_this_second = 0;
            next_report += std::chrono::seconds(1);
        }
    }

    std::cout << "total: " << ok << " ok, " << failed << " failed\n";
    if (first_failure.has_value() && failed_this_second > 0) {
        std::cout << "WARNING: requests were still failing when the run ended, so the window\n"
                     "below is cut short. Rerun with a longer --seconds or close the window earlier.\n";
    }
    if (first_failure.has_value()) {
        double window_ms =
            std::chrono::duration<double, std::milli>(*last_failure - *first_failure).count();
        std::cout << std::fixed << std::setprecision(0)
                  << "requests failed for " << window_ms
                  << " ms (first to last failure); detection setting = "
                  << o.interval_ms * o.misses << " ms\n";
    } else {
        std::cout << "no failures seen (did you close a server window?)\n";
    }
}

}//namespace

int main(int argc, char* argv[]) {
    try {
        Options o = parse_options(argc, argv);
        if (o.mode == "load") {
            run_load(o);
        } else if (o.mode == "failover") {
            run_failover(o);
        } else {
            throw std::invalid_argument("mode must be 'load' or 'failover'");
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
