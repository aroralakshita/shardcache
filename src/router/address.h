#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace shardcache {

inline std::pair<std::string, std::uint16_t> split_address(const std::string& address) {
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

}
