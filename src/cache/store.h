#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <mutex>

namespace shardcache {

// a single-node, in-memory key-value store with LRU eviction and lazy TTL expiry

class Store {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    using NowFn = std::function<TimePoint()>;

    explicit Store(std::size_t capacity, NowFn now = Clock::now);

    void set(const std::string& key, const std::string& value,
             std::optional<std::chrono::milliseconds> ttl = std::nullopt);

    std::optional<std::string> get(const std::string& key);

    bool remove(const std::string& key);

    std::size_t size() const;

private:
    struct Entry {
        std::string key;
        std::string value;
        std::optional<TimePoint> expires_at;
    };

    bool is_expired(const Entry& entry) const;

    std::size_t capacity_;
    NowFn now_;

    std::list<Entry> order_;

    std::unordered_map<std::string, std::list<Entry>::iterator> index_;

    mutable std::mutex mutex_;

    bool remove_locked(const std::string& key);
    


};
}
