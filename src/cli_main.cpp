#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

#include "net/line_reader.h"
#include "net/socket.h"

int main(int argc, char* argv[]) {
    try {
        std::string host = "127.0.0.1";
        std::uint16_t port = 7000;
        if (argc > 1) {
            host = argv[1];
        }
        if (argc > 2) {
            port = static_cast<std::uint16_t>(std::stoi(argv[2]));
        }

        shardcache::net::Socket connection = shardcache::net::Socket::connect_to(host, port);
        shardcache::net::LineReader reader(connection);
        std::cout << "connected to " << host << ":" << port << "\n> " << std::flush;

        std::string line;
        while (std::getline(std::cin, line)) {
            connection.send_all(line + "\n");
            std::optional<std::string> response = reader.read_line();
            if (!response.has_value()) {
                std::cout << "server closed the connection\n";
                break;
            }
            std::cout << *response << "\n> " << std::flush;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
