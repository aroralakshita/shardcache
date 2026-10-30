#include "server/execute.h"

namespace shardcache {

std::string execute(const Command& command, Store& store) {

    if (command.type == CommandType::Set) {
        store.set(command.key, command.value, command.ttl);
        return "OK";
    }

    if (command.type == CommandType::Get) {
        std::optional<std::string> value = store.get(command.key);
        if (!value.has_value()) {
            return "NOT_FOUND";
        }

        return "VALUE " + *value;
    }

    if (command.type == CommandType::Del) {
        if (store.remove(command.key)) {
            return "DELETED";
        }

        return "NOT_FOUND";

    }

    if (command.type == CommandType::Ping) {
        return "PONG";
    }

    return "ERROR unknown command";
}

}
