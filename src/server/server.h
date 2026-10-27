#pragma once

#include <cstdint>

#include "cache/store.h"
#include "net/socket.h"

namespace shardcache {

class Server {
public:
    Server(std::uint16_t port, Store& store);

    std::uint16_t port() const;

    void serve_one_client();

    void run();

private:
    void serve_client(net::Socket& client);

    net::Socket listener_;
    Store& store_;
};

}
