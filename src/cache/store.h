#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

namespace shardcache {

// a single-node, in-memory key-value store.

class Store {
public:

    void set(const std::string& key, const std::string& value);

    std::optional<std::string> get(const std::string& key) const;

    bool remove(const std::string& key);

    std::size_t size() const;

private:
    std::unordered_map<std::string, std::string> data_;
};

}
