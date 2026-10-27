#include "net/line_reader.h"

#include <stdexcept>

namespace shardcache::net {

std::optional<std::string> LineReader::read_line() {
    while (true) {
        std::size_t newline = buffer_.find('\n');
        if (newline != std::string::npos) {
            std::string line = buffer_.substr(0, newline);
            buffer_.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        if (buffer_.size() > kMaxLineLength) {
            throw std::runtime_error("line too long");
        }

        std::string chunk = socket_.receive_some();
        if (chunk.empty()) {
            return std::nullopt;
        }
        buffer_ += chunk;
    }
}

}
