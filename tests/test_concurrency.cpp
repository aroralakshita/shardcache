#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

#include "cache/store.h"

using shardcache::Store;
using namespace std::chrono_literals;

namespace {

constexpr int kThreads = 8;
constexpr int kOpsPerThread = 20000;

}

TEST(ConcurrencyTest, ManyThreadsMixedOperationsKeepStoreConsistent) {
    Store s(100); 
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&s, t] {
            for (int i = 0; i < kOpsPerThread; ++i) {
                std::string key = "k" + std::to_string((i * 7 + t) % 300);
                s.set(key, "v" + std::to_string(t));
                s.get(key);
                if (i % 5 == 0) {
                    s.remove(key);
                }
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    EXPECT_LE(s.size(), 100u);  
}

TEST(ConcurrencyTest, ConcurrentWritesToDistinctKeysAreAllKept) {
    Store s(kThreads * 1000); 
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&s, t] {
            for (int i = 0; i < 1000; ++i) {
                s.set("t" + std::to_string(t) + ":" + std::to_string(i), "x");
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(s.size(), static_cast<std::size_t>(kThreads * 1000));  
}

TEST(ConcurrencyTest, ConcurrentExpiryDoesNotDeadlockOrCrash) {
    Store s(1000);
    for (int i = 0; i < 500; ++i) {
        s.set("k" + std::to_string(i), "v", 1ms); 
    }
    std::this_thread::sleep_for(5ms);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&s] {
            for (int i = 0; i < 500; ++i) {
                s.get("k" + std::to_string(i));  
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(s.size(), 0u);
}