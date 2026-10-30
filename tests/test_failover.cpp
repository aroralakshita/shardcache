#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "cache/store.h"
#include "net/socket.h"
#include "router/failure_detector.h"
#include "router/router.h"
#include "server/server.h"

using namespace shardcache;
using namespace std::chrono_literals;

namespace {

std::string address_of(const Server& server) {
    return "127.0.0.1:" + std::to_string(server.port());
}

std::string dead_address() {
    net::Socket probe = net::Socket::listen_on(0);
    return "127.0.0.1:" + std::to_string(probe.local_port());
} 

bool wait_until(const std::function<bool()>& condition, std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(10ms);
    }
    return condition();
}

}

//the detector on its own
TEST(FailureDetectorTest, HealthyNodeStaysAlive) {
    Store store(100);
    Server node(0, store);
    std::thread t([&node] { node.serve_one_client(); });
    {
        FailureDetector detector({address_of(node)}, 200ms, 3);
        for (int i = 0; i < 5; ++i) {
            detector.check_once();
        }
        EXPECT_TRUE(detector.is_alive(address_of(node)));
    }  
    t.join();
}

TEST(FailureDetectorTest, NodeIsMarkedDownOnlyAfterKMisses) {
    std::string dead = dead_address();
    FailureDetector detector({dead}, 50ms, 3);
    detector.check_once();
    detector.check_once();
    EXPECT_TRUE(detector.is_alive(dead)) << "2 misses must not be enough (false-positive guard)";
    detector.check_once();
    EXPECT_FALSE(detector.is_alive(dead));
}

TEST(FailureDetectorTest, UnknownNodesAreConsideredAlive) {
    FailureDetector detector({}, 50ms, 3);
    EXPECT_TRUE(detector.is_alive("127.0.0.1:1"));
}

//end to end - a node dies, the router reroutes 

TEST(FailoverTest, ReadsAndWritesRerouteWhenANodeDies) {
    Store store1(1000), store2(1000), store3(1000);
    Server node1(0, store1), node2(0, store2), node3(0, store3);
    std::vector<Server*> servers = {&node1, &node2, &node3};

    std::vector<std::thread> threads;
    for (Server* server : servers) {
        threads.emplace_back([server] { server->serve_one_client(); });
        threads.emplace_back([server] { server->serve_one_client(); });
    }

    std::vector<std::string> nodes = {address_of(node1), address_of(node2), address_of(node3)};
    std::map<std::string, Server*> server_of = {
        {nodes[0], &node1}, {nodes[1], &node2}, {nodes[2], &node3}};

    {
        Router router(nodes, 2);
        router.enable_failure_detection(100ms, 3);

        const int kKeys = 100;
        for (int i = 0; i < kKeys; ++i) {
            ASSERT_EQ(router.request("SET key:" + std::to_string(i) + " v" + std::to_string(i)), "OK");
        }

        std::string victim = *router.owner_of("key:0");
        server_of[victim]->simulate_failure();

        ASSERT_TRUE(wait_until([&] { return !router.is_alive(victim); }, 5000ms))
            << "the detector never marked " << victim << " DOWN";

        for (int i = 0; i < kKeys; ++i) {
            EXPECT_EQ(router.request("GET key:" + std::to_string(i)), "VALUE v" + std::to_string(i));
        }

        for (int i = 0; i < kKeys; ++i) {
            std::vector<std::string> live = router.live_replicas_of("key:" + std::to_string(i));
            EXPECT_EQ(live.size(), 2u);
            for (const std::string& node : live) {
                EXPECT_NE(node, victim);
            }
        }
        EXPECT_EQ(router.request("SET new:key hello"), "OK");
        EXPECT_EQ(router.request("GET new:key"), "VALUE hello");
    }  

    for (std::thread& thread : threads) {
        thread.join();
    }
}

TEST(FailoverTest, LiveReplicasFollowThePreferenceList) {
    Store store1(100), store2(100), store3(100);
    Server node1(0, store1), node2(0, store2), node3(0, store3);
    std::thread t1([&node1] { node1.serve_one_client(); });
    std::thread t2([&node2] { node2.serve_one_client(); });
    std::thread t3([&node3] { node3.serve_one_client(); });

    {
        Router router({address_of(node1), address_of(node2), address_of(node3)}, 2);
        for (int i = 0; i < 100; ++i) {
            std::string key = "k" + std::to_string(i);
            EXPECT_EQ(router.live_replicas_of(key), router.replicas_of(key));
        }
    }

    t1.join();
    t2.join();
    t3.join();
}
