#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "net/line_reader.h"
#include "net/socket.h"
#include "ring/hash_ring.h"

namespace shardcache {

class Router {
public:
    explicit Router(const std::vector<std::string>& nodes, int virtual_nodes = 1000);

    std::string request(const std::string& line);

    std::optional<std::string> owner_of(const std::string& key) const;

private:
    struct Connection {
        explicit Connection(net::Socket s) : socket(std::move(s)), reader(socket) {}
        net::Socket socket;
        net::LineReader reader;
    };

    std::string send_to(const std::string& node, const std::string& line);

    HashRing ring_;

    std::map<std::string, std::unique_ptr<Connection>> connections_;
};

}
