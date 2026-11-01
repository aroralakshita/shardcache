#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "net/socket.h"

namespace shardcache::net {

//turns a TCP byte stream into lines. throws if a line is longer than the maximum
class LineReader {
public:
    static constexpr std::size_t kMaxLineLength = 1024 * 1024;

    explicit LineReader(const Socket& socket, std::size_t max_line_length = kMaxLineLength)
        : socket_(socket), max_line_length_(max_line_length) {}

    std::optional<std::string> read_line();

private:
    const Socket& socket_;
    std::size_t max_line_length_;
    std::string buffer_;
};

}
