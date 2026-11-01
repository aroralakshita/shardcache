#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>

#include "net/line_reader.h"
#include "net/socket.h"

using shardcache::net::LineReader;
using shardcache::net::Socket;

namespace {

constexpr std::size_t kLimit = 16;

//a connected pair of sockets: write to `client`, read from `server`.
struct Connection {
    Socket client;
    Socket server;
};

Connection connect_pair() {
    Socket listener = Socket::listen_on(0);
    Socket client = Socket::connect_to("127.0.0.1", listener.local_port());
    Socket server = listener.accept();
    return Connection{std::move(client), std::move(server)};
}

} //namespace

TEST(LineReaderTest, LineExactlyAtLimitIsAccepted) {
    Connection c = connect_pair();
    LineReader reader(c.server, kLimit);
    c.client.send_all(std::string(kLimit, 'a') + "\n");
    EXPECT_EQ(reader.read_line(), std::string(kLimit, 'a'));
}

TEST(LineReaderTest, LineOneByteOverLimitIsRejected) {
    Connection c = connect_pair();
    LineReader reader(c.server, kLimit);
    c.client.send_all(std::string(kLimit + 1, 'a') + "\n");
    EXPECT_THROW(reader.read_line(), std::runtime_error);
}

TEST(LineReaderTest, OversizedLineWithNewlineInSameChunkIsRejected) {
    Connection c = connect_pair();
    LineReader reader(c.server, kLimit);
    c.client.send_all(std::string(kLimit * 2, 'a') + "\n");
    EXPECT_THROW(reader.read_line(), std::runtime_error);
}

TEST(LineReaderTest, ValidFirstLineIsReturnedBeforeOversizedSecondLine) {
    Connection c = connect_pair();
    LineReader reader(c.server, kLimit);
    c.client.send_all("ok\n" + std::string(kLimit * 2, 'a') + "\n");
    EXPECT_EQ(reader.read_line(), "ok");
    EXPECT_THROW(reader.read_line(), std::runtime_error);
}

TEST(LineReaderTest, CrLfLineAtLimitIsAccepted) {
    Connection c = connect_pair();
    LineReader reader(c.server, kLimit);
    c.client.send_all(std::string(kLimit, 'a') + "\r\n");
    EXPECT_EQ(reader.read_line(), std::string(kLimit, 'a'));
}

TEST(LineReaderTest, DisconnectReturnsNullopt) {
    Connection c = connect_pair();
    LineReader reader(c.server, kLimit);
    c.client.close();
    EXPECT_FALSE(reader.read_line().has_value());
}
