#include <gtest/gtest.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ring/hash.h"
#include "ring/hash_ring.h"

using shardcache::hash64;
using shardcache::HashRing;

namespace {

std::vector<std::string> make_keys(int count) {
    std::vector<std::string> keys;
    for (int i = 0; i < count; ++i) {
        keys.push_back("key:" + std::to_string(i));
    }
    return keys;
}

std::string node_name(int i) {
    return "127.0.0.1:" + std::to_string(7001 + i);
}

}

//behavior

TEST(RingTest, EmptyRingHasNoOwner) {
    HashRing ring;
    EXPECT_FALSE(ring.get_node("a").has_value());
}

TEST(RingTest, SingleNodeOwnsEveryKey) {
    HashRing ring;
    ring.add_node("A");
    for (const std::string& key : make_keys(1000)) {
        EXPECT_EQ(ring.get_node(key), "A");
    }
}

TEST(RingTest, SameKeyAlwaysMapsToSameNode) {
    HashRing ring;
    ring.add_node("A");
    ring.add_node("B");
    ring.add_node("C");
    for (const std::string& key : make_keys(1000)) {
        EXPECT_EQ(ring.get_node(key), ring.get_node(key));
    }
}

TEST(RingTest, TwoRingsWithSameNodesAgree) {
    HashRing ring1;
    HashRing ring2;
    for (const char* node : {"A", "B", "C"}) {
        ring1.add_node(node);
    }
    for (const char* node : {"C", "A", "B"}) {
        ring2.add_node(node);
    }
    for (const std::string& key : make_keys(1000)) {
        EXPECT_EQ(ring1.get_node(key), ring2.get_node(key));
    }
}

TEST(RingTest, AddingSameNodeTwiceDoesNothing) {
    HashRing ring;
    ring.add_node("A");
    ring.add_node("B");
    std::map<std::string, std::string> before;
    for (const std::string& key : make_keys(1000)) {
        before[key] = *ring.get_node(key);
    }
    ring.add_node("A");
    EXPECT_EQ(ring.node_count(), 2u);
    for (const std::string& key : make_keys(1000)) {
        EXPECT_EQ(ring.get_node(key), before[key]);
    }
}

TEST(RingTest, RemovingUnknownNodeDoesNothing) {
    HashRing ring;
    ring.add_node("A");
    ring.remove_node("Z");
    EXPECT_EQ(ring.node_count(), 1u);
    EXPECT_EQ(ring.get_node("a"), "A");
}

TEST(RingTest, RemovingLastNodeEmptiesRing) {
    HashRing ring;
    ring.add_node("A");
    ring.remove_node("A");
    EXPECT_EQ(ring.node_count(), 0u);
    EXPECT_FALSE(ring.get_node("a").has_value());
}

TEST(RingTest, RemovingNodeOnlyMovesItsOwnKeysAndSpreadsThem) {
    HashRing ring;
    for (int i = 0; i < 4; ++i) {
        ring.add_node(node_name(i));
    }
    std::vector<std::string> keys = make_keys(10000);
    std::map<std::string, std::string> before;
    for (const std::string& key : keys) {
        before[key] = *ring.get_node(key);
    }

    const std::string removed = node_name(0);
    ring.remove_node(removed);

    std::map<std::string, int> received;  
    for (const std::string& key : keys) {
        std::string now = *ring.get_node(key);
        if (before[key] == removed) {
            EXPECT_NE(now, removed);
            ++received[now];
        } else {
            EXPECT_EQ(now, before[key]) << "key " << key << " moved but its node wasn't removed";
        }
    }
    EXPECT_EQ(received.size(), 3u);
}

//measurements

TEST(RingMeasure, DistributionByVirtualNodeCount) {
    const int kNodes = 5;
    std::vector<std::string> keys = make_keys(100000);
    const double ideal = static_cast<double>(keys.size()) / kNodes;

    for (int vnodes : {1, 10, 100, 1000}) {
        HashRing ring(vnodes);
        for (int i = 0; i < kNodes; ++i) {
            ring.add_node(node_name(i));
        }
        std::map<std::string, int> counts;
        for (const std::string& key : keys) {
            ++counts[*ring.get_node(key)];
        }
        int max_count = 0;
        int min_count = static_cast<int>(keys.size());
        for (const auto& entry : counts) {
            max_count = std::max(max_count, entry.second);
            min_count = std::min(min_count, entry.second);
        }
        std::cout << std::fixed << std::setprecision(1) << "vnodes=" << std::setw(4) << vnodes
                  << "  busiest node: " << 100.0 * max_count / ideal << "% of fair share"
                  << "  quietest: " << 100.0 * min_count / ideal << "%\n";

        if (vnodes == 100) {
            EXPECT_LT(max_count, ideal * 1.5);
        }
        if (vnodes == 1000) {
            EXPECT_LT(max_count, ideal * 1.2);
        }
    }
}

TEST(RingMeasure, RemappedKeysWhenAddingANodeVsModulo) {
    const int kNodesBefore = 3;
    std::vector<std::string> keys = make_keys(100000);

    HashRing ring;
    for (int i = 0; i < kNodesBefore; ++i) {
        ring.add_node(node_name(i));
    }
    std::map<std::string, std::string> before;
    for (const std::string& key : keys) {
        before[key] = *ring.get_node(key);
    }
    const std::string added = node_name(kNodesBefore);
    ring.add_node(added);

    int ring_moved = 0;
    int modulo_moved = 0;
    for (const std::string& key : keys) {
        std::string now = *ring.get_node(key);
        if (now != before[key]) {
            ++ring_moved;
            EXPECT_EQ(now, added) << "a key moved between two OLD nodes";
        }
        std::uint64_t h = hash64(key);
        if (h % kNodesBefore != h % (kNodesBefore + 1)) {
            ++modulo_moved;
        }
    }

    double ring_pct = 100.0 * ring_moved / static_cast<double>(keys.size());
    double modulo_pct = 100.0 * modulo_moved / static_cast<double>(keys.size());
    std::cout << std::fixed << std::setprecision(1) << "adding node " << kNodesBefore + 1
              << ": consistent hashing moved " << ring_pct << "% of keys, modulo moved "
              << modulo_pct << "%\n";

    EXPECT_LT(ring_pct, 35.0);
    EXPECT_GT(modulo_pct, 65.0);
}

//replica placement

TEST(ReplicaTest, EmptyRingHasNoReplicas) {
    HashRing ring;
    EXPECT_TRUE(ring.get_nodes("a", 2).empty());
}

TEST(ReplicaTest, FirstReplicaIsThePrimary) {
    HashRing ring;
    for (int i = 0; i < 3; ++i) {
        ring.add_node(node_name(i));
    }
    for (const std::string& key : make_keys(1000)) {
        std::vector<std::string> replicas = ring.get_nodes(key, 2);
        ASSERT_EQ(replicas.size(), 2u);
        EXPECT_EQ(replicas[0], *ring.get_node(key));
    }
}

TEST(ReplicaTest, ReplicasAreDistinctPhysicalNodes) {
    HashRing ring;
    for (int i = 0; i < 4; ++i) {
        ring.add_node(node_name(i));
    }
    for (const std::string& key : make_keys(1000)) {
        std::vector<std::string> replicas = ring.get_nodes(key, 3);
        std::set<std::string> unique(replicas.begin(), replicas.end());
        EXPECT_EQ(unique.size(), 3u) << "duplicate replica for " << key;
    }
}

TEST(ReplicaTest, AskingForMoreReplicasThanNodesReturnsEveryNodeOnce) {
    HashRing ring;
    ring.add_node("A");
    ring.add_node("B");
    std::vector<std::string> replicas = ring.get_nodes("a", 5); 
    std::set<std::string> unique(replicas.begin(), replicas.end());
    EXPECT_EQ(replicas.size(), 2u);
    EXPECT_EQ(unique.size(), 2u);
}

TEST(ReplicaTest, ReplicaSetsAgreeAcrossRings) {
    HashRing ring1;
    HashRing ring2;
    for (const char* node : {"A", "B", "C"}) {
        ring1.add_node(node);
    }
    for (const char* node : {"B", "C", "A"}) {
        ring2.add_node(node);
    }
    for (const std::string& key : make_keys(500)) {
        EXPECT_EQ(ring1.get_nodes(key, 2), ring2.get_nodes(key, 2));
    }
}
