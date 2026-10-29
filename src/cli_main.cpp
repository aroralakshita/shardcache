#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "protocol/protocol.h"
#include "router/router.h"

int main(int argc, char* argv[]) {
    try {
        std::vector<std::string> nodes;
        for (int i = 1; i < argc; ++i) {
            nodes.push_back(argv[i]);
        }
        if (nodes.empty()) {
            nodes.push_back("127.0.0.1:7000");
        }

        shardcache::Router router(nodes);
        std::cout << "connected to " << nodes.size() << " node(s)\n> " << std::flush;

        std::string line;
        while (std::getline(std::cin, line)) {
            std::string reply = router.request(line);
            shardcache::ParseResult parsed = shardcache::parse_command(line);
            if (parsed.command.has_value()) {
                std::cout << "[" << router.owner_of(parsed.command->key).value_or("?") << "] ";
            }
            std::cout << reply << "\n> " << std::flush;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
