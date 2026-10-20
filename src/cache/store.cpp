#include "cache/store.h"

namespace shardcache {

void Store::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::optional<std::string> Store::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool Store::remove(const std::string& key) {
    return data_.erase(key) == 1;
}

std::size_t Store::size() const {
    return data_.size();
}

}
