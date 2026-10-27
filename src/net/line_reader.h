#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "net/socket.h"

namespace shardcache::net {

class LineReader {
public:
    explicit LineReader(const Socket& socket) : socket_(socket) {}

    std::optional<std::string> read_line();

    static constexpr std::size_t kMaxLineLength = 1024 * 1024;

private:
    const Socket& socket_;
    std::string buffer_;
};

}
