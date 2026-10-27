#include "cache/store.h"

namespace shardcache {

Store::Store(std::size_t capacity, NowFn now) : capacity_(capacity), now_(now) {}

bool Store::is_expired(const Entry& entry) const {
    if (!entry.expires_at.has_value()) {
        return false;
    }

    return now_() > *entry.expires_at;
}

void Store::set(const std::string& key, const std::string& value,
                std::optional<std::chrono::milliseconds> ttl) {
    std::lock_guard<std::mutex> lock(mutex_);                
    if (capacity_ == 0) {
        return;
    }

    std::optional<Store::TimePoint> deadline;
    if (ttl.has_value()) {
        deadline = now_() + *ttl;
    }

    auto it = index_.find(key);
    if (it != index_.end()) {
        it-> second->value= value;
        it->second->expires_at = deadline;
        order_.splice(order_.begin(), order_, it->second);
        return;
    }

    //case b:new key
    if (order_.size() == capacity_) {
        index_.erase(order_.back().key);
        order_.pop_back();
    }
    
    order_.push_front({key, value, deadline});
    index_[key] = order_.begin();
}

std::optional<std::string> Store::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = index_.find(key);
    if (it == index_.end()) {
        return std::nullopt;
    }

    if (is_expired(*it->second)) {
        remove_locked(key);
        return std::nullopt;

    }

    order_.splice(order_.begin(), order_, it->second);
    return it->second->value;
}

bool Store::remove_locked(const std::string& key) {
    auto it = index_.find(key);
    if (it == index_.end()) {
        return false;
    }

    order_.erase(it->second);
    index_.erase(key);
    return true;
}

std::size_t Store::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return order_.size();
}

bool Store::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return remove_locked(key);
}
}