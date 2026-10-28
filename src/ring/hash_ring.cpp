#include "ring/hash_ring.h"

#include "ring/hash.h"

namespace shardcache {

HashRing::HashRing(int virtual_nodes) : virtual_nodes_(virtual_nodes) {}

void HashRing::add_node(const std::string& node) {
    if (nodes_.contains(node)) {
        return;
    }
    nodes_.insert(node);
   
    for (int i = 0; i < virtual_nodes_; ++i) {
        std::uint64_t position = hash64(node + "#" + std::to_string(i));
        ring_[position] = node;
    }
}

void HashRing::remove_node(const std::string& node) {
    if (!nodes_.contains(node)) {
        return;
    }
    nodes_.erase(node);
    for (int i = 0; i < virtual_nodes_; ++i) {
        std::uint64_t position = hash64(node + "#" + std::to_string(i));
        ring_.erase(position);
}
}

std::optional<std::string> HashRing::get_node(const std::string& key) const {
    if (ring_.empty()) {
    return std::nullopt;
    }

    auto it = ring_.lower_bound(hash64(key));

    if (it == ring_.end()) {
        it = ring_.begin();
    }

    return it->second;
}

std::size_t HashRing::node_count() const {
    return nodes_.size();
}
}
