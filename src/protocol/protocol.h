#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace shardcache {

enum class CommandType { Get, Set, Del };

struct Command {
    CommandType type = CommandType::Get;
    std::string key;
    std::string value;                      
    std::optional<std::chrono::seconds> ttl;  
};

struct ParseResult {
    std::optional<Command> command; 
    std::string error;             
};

inline constexpr std::chrono::seconds kMaxTtl{10LL * 365 * 24 * 60 * 60};

ParseResult parse_command(std::string_view line);

}
