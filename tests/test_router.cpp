#include <gtest/gtest.h>

#include <map>
#include <string>
#include <thread>
#include <vector>

#include "cache/store.h"
#include "net/socket.h"
#include "router/router.h"
#include "server/server.h"

using namespace shardcache;

namespace {

std::string address_of(const Server& server) {
    return "127.0.0.1:" + std::to_string(server.port());
}

}

TEST(RouterTest, EveryKeyLandsOnItsOwnerAndOnlyThere) {
    Store store1(1000), store2(1000), store3(1000);
    Server node1(0, store1), node2(0, store2), node3(0, store3);
    std::thread t1([&node1] { node1.serve_one_client(); });
    std::thread t2([&node2] { node2.serve_one_client(); });
    std::thread t3([&node3] { node3.serve_one_client(); });

    std::vector<std::string> nodes = {address_of(node1), address_of(node2), address_of(node3)};
    std::map<std::string, Store*> store_of = {
        {nodes[0], &store1}, {nodes[1], &store2}, {nodes[2], &store3}};

    {
        Router router(nodes);
        const int kKeys = 300;
        for (int i = 0; i < kKeys; ++i) {
            std::string n = std::to_string(i);
            EXPECT_EQ(router.request("SET key:" + n + " value" + n), "OK");
        }
        for (int i = 0; i < kKeys; ++i) {
            std::string n = std::to_string(i);
            EXPECT_EQ(router.request("GET key:" + n), "VALUE value" + n);
        }

        for (int i = 0; i < kKeys; ++i) {
            std::string key = "key:" + std::to_string(i);
            std::string owner = *router.owner_of(key);
            for (const std::string& node : nodes) {
                bool has_key = store_of[node]->get(key).has_value();
                EXPECT_EQ(has_key, node == owner) << key << " on " << node;
            }
        }

        EXPECT_GT(store1.size(), 0u);
        EXPECT_GT(store2.size(), 0u);
        EXPECT_GT(store3.size(), 0u);
    }

    t1.join();
    t2.join();
    t3.join();
}

TEST(RouterTest, DeleteGoesToTheSameOwner) {
    Store store1(100), store2(100);
    Server node1(0, store1), node2(0, store2);
    std::thread t1([&node1] { node1.serve_one_client(); });
    std::thread t2([&node2] { node2.serve_one_client(); });

    {
        Router router({address_of(node1), address_of(node2)});
        EXPECT_EQ(router.request("SET user:42 alice"), "OK");
        EXPECT_EQ(router.request("DEL user:42"), "DELETED");
        EXPECT_EQ(router.request("GET user:42"), "NOT_FOUND");
    }

    t1.join();
    t2.join();
}

TEST(RouterTest, InvalidCommandIsRejectedLocally) {
    Store store(100);
    Server node(0, store);
    std::thread t([&node] { node.serve_one_client(); });

    {
        Router router({address_of(node)});
        EXPECT_EQ(router.request("FLY a").rfind("ERROR", 0), 0u);
        EXPECT_EQ(router.request("").rfind("ERROR", 0), 0u);
        EXPECT_EQ(router.request("SET a 1"), "OK");
    }

    t.join();
}

TEST(RouterTest, NoNodesMeansError) {
    Router router(std::vector<std::string>{});
    EXPECT_EQ(router.request("GET a").rfind("ERROR", 0), 0u);
}

TEST(RouterTest, UnreachableNodeFailsAtStartup) {
    std::uint16_t dead_port = 0;
    {
        net::Socket probe = net::Socket::listen_on(0);
        dead_port = probe.local_port();
    }
    EXPECT_THROW(Router({"127.0.0.1:" + std::to_string(dead_port)}), std::exception);
}
