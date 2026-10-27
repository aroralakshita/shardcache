#include "protocol/protocol.h"

#include <cctype>
#include <charconv>
#include <utility>
#include <vector>

namespace shardcache {

namespace {

std::vector<std::string_view> split(std::string_view line) {
    std::vector<std::string_view> parts;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && line[i] == ' ') {
            ++i;
        }
        std::size_t start = i;
        while (i < line.size() && line[i] != ' ') {
            ++i;
        }
        if (i > start) {
            parts.push_back(line.substr(start, i - start));
        }
    }
    return parts;
}

std::string to_upper(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

ParseResult fail(std::string reason) {
    return ParseResult{std::nullopt, std::move(reason)};
}

std::optional<std::chrono::seconds> parse_ttl(std::string_view text) {
    long long seconds = 0;
    const char* end = text.data() + text.size();
    std::from_chars_result result = std::from_chars(text.data(), end, seconds);
    if (result.ec != std::errc{} || result.ptr != end || seconds <= 0 ||
        seconds > kMaxTtl.count()) {
        return std::nullopt;
    }
    return std::chrono::seconds(seconds);
}

}

ParseResult parse_command(std::string_view line) {
    std::vector<std::string_view> parts = split(line);
    if (parts.empty()) {
        return fail("empty command");
    }

    std::string name = to_upper(parts[0]);
    Command command;

    if (name == "GET" || name == "DEL") {
        if (parts.size() != 2) {
            return fail("usage: " + name + " <key>");
        }
        command.type = (name == "GET") ? CommandType::Get : CommandType::Del;
        command.key = std::string(parts[1]);
        return ParseResult{command, ""};
    }

    if (name == "SET") {
        if (parts.size() != 3 && parts.size() != 4) {
            return fail("usage: SET <key> <value> [ttl_seconds]");
        }
        command.type = CommandType::Set;
        command.key = std::string(parts[1]);
        command.value = std::string(parts[2]);
        if (parts.size() == 4) {
            command.ttl = parse_ttl(parts[3]);
            if (!command.ttl.has_value()) {
                return fail("ttl must be a whole number of seconds between 1 and " +
                            std::to_string(kMaxTtl.count()));
            }
        }
        return ParseResult{command, ""};
    }

    return fail("unknown command '" + std::string(parts[0]) + "'");
}

}
