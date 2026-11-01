#include "net/line_reader.h"

#include <stdexcept>

namespace shardcache::net {

std::optional<std::string> LineReader::read_line() {
    while (true) {
        std::size_t newline = buffer_.find('\n');
        if (newline != std::string::npos) {
            std::size_t length = newline;
            if (length > 0 && buffer_[length - 1] == '\r') {
                --length;
            }
            if (length > max_line_length_) {
                throw std::runtime_error("line too long");
            }
            std::string line = buffer_.substr(0, length);
            buffer_.erase(0, newline + 1);
            return line;
        }
        // no newline yet: +1 leaves room for a '\r' whose '\n' hasn't arrived
        if (buffer_.size() > max_line_length_ + 1) {
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
