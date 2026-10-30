#include "router/router.h"

#include <cstdint>
#include <exception>
#include <stdexcept>

#include "protocol/protocol.h"

namespace shardcache {

namespace {

std::pair<std::string, std::uint16_t> split_address(const std::string& address) {
    std::size_t colon = address.rfind(':');
    if (colon == std::string::npos) {
        throw std::invalid_argument("expected host:port, got '" + address + "'");
    }
    int port = std::stoi(address.substr(colon + 1));
    if (port <= 0 || port > 65535) {
        throw std::invalid_argument("bad port in '" + address + "'");
    }
    return {address.substr(0, colon), static_cast<std::uint16_t>(port)};
}

bool is_error(const std::string& reply) {
    return reply.rfind("ERROR", 0) == 0;
}

}

Router::Router(const std::vector<std::string>& nodes, std::size_t replication_factor,
               int virtual_nodes)
    : replication_factor_(replication_factor), ring_(virtual_nodes) {
    for (const std::string& node : nodes) {
        std::pair<std::string, std::uint16_t> address = split_address(node);
        net::Socket socket = net::Socket::connect_to(address.first, address.second);
        connections_[node] = std::make_unique<Connection>(std::move(socket));
        ring_.add_node(node);
    }
}

std::optional<std::string> Router::owner_of(const std::string& key) const {
    return ring_.get_node(key);
}

std::vector<std::string> Router::replicas_of(const std::string& key) const {
    return ring_.get_nodes(key, replication_factor_);
}

std::string Router::send_to(const std::string& node, const std::string& line) {
    auto it = connections_.find(node);
    if (it == connections_.end()) {
        return "ERROR unknown node " + node;
    }
    Connection& connection = *it->second;
    try {
        connection.socket.send_all(line + "\n");
        std::optional<std::string> reply = connection.reader.read_line();
        if (!reply.has_value()) {
            return "ERROR node " + node + " closed the connection";
        }
        return *reply;
    } catch (const std::exception& e) {
        return "ERROR node " + node + " unavailable: " + e.what();
    }
}

std::string Router::request(const std::string& line) {
    ParseResult parsed = parse_command(line);
    if (!parsed.command.has_value()) {
        return "ERROR " + parsed.error;
    }

    std::vector<std::string> replicas = replicas_of(parsed.command->key);
    if (replicas.empty()) {
        return "ERROR no nodes";
    }

    if (parsed.command->type == CommandType::Get) {
        return send_to(replicas[0], line);
    }

    std::string primary_reply;

    for (const std::string& node : replicas) {
        std::string reply = send_to(node, line);

        if (is_error(reply)) {
            return "ERROR write failed on " + node + ": " + reply;
        }

        if (node == replicas[0]) {
            primary_reply = reply;
        }
    }
    return primary_reply;

}
}
