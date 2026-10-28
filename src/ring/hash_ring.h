#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>

namespace shardcache {
class HashRing {
public:
    explicit HashRing(int virtual_nodes = 100);

    void add_node(const std::string& node);

    void remove_node(const std::string& node);

    std::optional<std::string> get_node(const std::string& key) const;

    std::size_t node_count() const;

private:
    int virtual_nodes_;

    std::map<std::uint64_t, std::string> ring_;

    std::set<std::string> nodes_;
};

}
