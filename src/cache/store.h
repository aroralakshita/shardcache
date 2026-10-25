#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace shardcache {

// a single-node, in-memory key-value store with LRU eviction.

class Store {
public:
    explicit Store(std::size_t capacity);

    void set(const std::string& key, const std::string& value);

    std::optional<std::string> get(const std::string& key);

    bool remove(const std::string& key);

    std::size_t size() const;

private:
    using Entry = std::pair<std::string, std::string>;

    std::size_t capacity_;

    std::list<Entry> order_;

    std::unordered_map<std::string, std::list<Entry>::iterator> index_;
};

}
