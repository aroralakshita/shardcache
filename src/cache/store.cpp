#include "cache/store.h"

namespace shardcache {

Store::Store(std::size_t capacity) : capacity_(capacity) {}

void Store::set(const std::string& key, const std::string& value) {
    if (capacity_ == 0) {
        return;
    }

    auto it = index_.find(key);
    if (it != index_.end()) {
        it-> second->second = value;
        order_.splice(order_.begin(), order_, it->second);
        return;
    }

    //case b:new key
    if (order_.size() == capacity_) {
        index_.erase(order_.back().first);
        order_.pop_back();
    }
    
    order_.push_front({key, value});
    index_[key] = order_.begin();
}

std::optional<std::string> Store::get(const std::string& key) {
    auto it = index_.find(key);
    if (it == index_.end()) {
        return std::nullopt;
    }

    order_.splice(order_.begin(), order_, it->second);
    return it->second->second;
}

bool Store::remove(const std::string& key) {
    auto it = index_.find(key);
    if (it == index_.end()) {
        return false;
    }

    order_.erase(it->second);
    index_.erase(key);
    return true;
}

std::size_t Store::size() const {
    return order_.size();
}

}
