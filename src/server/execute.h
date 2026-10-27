#pragma once

#include <string>

#include "cache/store.h"
#include "protocol/protocol.h"

namespace shardcache {

std::string execute(const Command& command, Store& store);

}
