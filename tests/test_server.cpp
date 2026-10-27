#include <gtest/gtest.h>

#include <string>
#include <thread>

#include "cache/store.h"
#include "net/line_reader.h"
#include "net/socket.h"
#include "server/server.h"

using namespace shardcache;

namespace {

std::string request(net::Socket& connection, net::LineReader& reader, const std::string& line) {
    connection.send_all(line + "\n");
    return reader.read_line().value_or("<disconnected>");
}

}

TEST(ServerTest, SetGetDelOverTcp) {
    Store store(100);
    Server server(0, store);
    std::thread server_thread([&server] { server.serve_one_client(); });

    {
        net::Socket connection = net::Socket::connect_to("127.0.0.1", server.port());
        net::LineReader reader(connection);
        EXPECT_EQ(request(connection, reader, "SET a 1"), "OK");
        EXPECT_EQ(request(connection, reader, "GET a"), "VALUE 1");
        EXPECT_EQ(request(connection, reader, "DEL a"), "DELETED");
        EXPECT_EQ(request(connection, reader, "GET a"), "NOT_FOUND");
        EXPECT_EQ(request(connection, reader, "DEL a"), "NOT_FOUND");
        EXPECT_EQ(request(connection, reader, "SET b 2 60"), "OK");
        EXPECT_EQ(request(connection, reader, "GET b"), "VALUE 2");
    }

    server_thread.join();
}

TEST(ServerTest, InvalidCommandsGetErrorAndConnectionStaysUsable) {
    Store store(100);
    Server server(0, store);
    std::thread server_thread([&server] { server.serve_one_client(); });

    {
        net::Socket connection = net::Socket::connect_to("127.0.0.1", server.port());
        net::LineReader reader(connection);
        EXPECT_EQ(request(connection, reader, "FLY a").rfind("ERROR", 0), 0u);
        EXPECT_EQ(request(connection, reader, "SET a 1 abc").rfind("ERROR", 0), 0u);
        EXPECT_EQ(request(connection, reader, "SET a 1"), "OK");
    }

    server_thread.join();
}

TEST(ServerTest, FramingHandlesBatchedAndSplitCommands) {
    Store store(100);
    Server server(0, store);
    std::thread server_thread([&server] { server.serve_one_client(); });

    {
        net::Socket connection = net::Socket::connect_to("127.0.0.1", server.port());
        net::LineReader reader(connection);

        connection.send_all("SET a 1\nGET a\n");
        EXPECT_EQ(reader.read_line(), "OK");
        EXPECT_EQ(reader.read_line(), "VALUE 1");

        connection.send_all("GE");
        connection.send_all("T a\n");
        EXPECT_EQ(reader.read_line(), "VALUE 1");
    }

    server_thread.join();
}

TEST(ServerTest, TwoClientsAreServedAtTheSameTime) {
    Store store(100);
    Server server(0, store);
    std::thread worker_a([&server] { server.serve_one_client(); });
    std::thread worker_b([&server] { server.serve_one_client(); });

    {
        net::Socket a = net::Socket::connect_to("127.0.0.1", server.port());
        net::LineReader reader_a(a);
        EXPECT_EQ(request(a, reader_a, "SET k hello"), "OK");

        net::Socket b = net::Socket::connect_to("127.0.0.1", server.port());
        net::LineReader reader_b(b);
        EXPECT_EQ(request(b, reader_b, "GET k"), "VALUE hello");
        EXPECT_EQ(request(a, reader_a, "GET k"), "VALUE hello");
    }

    worker_a.join();
    worker_b.join();
}
