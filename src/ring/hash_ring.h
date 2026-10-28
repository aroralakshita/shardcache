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
    //default: 1000 virtual nodes per physical node
    //benchmark: busiest-node load was ~3% above fair share, vs. ~20% with 100
    explicit HashRing(int virtual_nodes = 1000);

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
