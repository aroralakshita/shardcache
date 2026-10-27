#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

#include "cache/store.h"
#include "server/server.h"

int main(int argc, char* argv[]) {
    try {
        std::uint16_t port = 7000;
        std::size_t capacity = 10000;
        if (argc > 1) {
            port = static_cast<std::uint16_t>(std::stoi(argv[1]));
        }
        if (argc > 2) {
            capacity = std::stoul(argv[2]);
        }

        shardcache::Store store(capacity);
        shardcache::Server server(port, store);
        std::cout << "ShardCache listening on 127.0.0.1:" << server.port()
                  << " (capacity " << capacity << " keys)\n";
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
