#include "server/server.h"

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include "net/line_reader.h"
#include "protocol/protocol.h"
#include "server/execute.h"

namespace shardcache {

Server::Server(std::uint16_t port, Store& store)
    : listener_(net::Socket::listen_on(port)), store_(store) {}

std::uint16_t Server::port() const {
    return listener_.local_port();
}

void Server::simulate_failure() {
    failed_ = true;
}

void Server::serve_one_client() {
    net::Socket client = listener_.accept();
    serve_client(client);
}

void Server::serve_client(net::Socket& client) {
    try {
        net::LineReader reader(client);
        while (true) {
            std::optional<std::string> line = reader.read_line();
            if (!line.has_value() || failed_) {
                break;
            }
            ParseResult parsed = parse_command(*line);
            std::string response = parsed.command.has_value()
                                       ? execute(*parsed.command, store_)
                                       : "ERROR " + parsed.error;
            client.send_all(response + "\n");
        }
    } catch (const std::exception& e) {
        std::cerr << "client error: " << e.what() << "\n";
    }
}

void Server::run() {
    while (true) {
        net::Socket client = listener_.accept();
        std::thread([this, c = std::move(client)]() mutable { serve_client(c); }).detach();
    }
}

}
