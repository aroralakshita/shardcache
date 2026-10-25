#include <iostream>
#include <string>

#include "cache/store.h"

void print_lookup(shardcache::Store& store, const std::string& key) {
    std::optional<std::string> value = store.get(key);
    if (value.has_value()) {
        std::cout << "GET " << key << " -> " << *value << "\n";
    } else {
        std::cout << "GET " << key << " -> (not found)\n";
    }
}

int main() {
    shardcache::Store store(100);

    store.set("user:1", "alice");
    store.set("user:2", "bob");
    print_lookup(store, "user:1");
    print_lookup(store, "user:3");

    store.set("user:1", "alicia");
    print_lookup(store, "user:1");

    std::cout << "DELETE user:2 -> " << (store.remove("user:2") ? "removed" : "absent") << "\n";
    print_lookup(store, "user:2");

    std::cout << "size = " << store.size() << "\n";
    return 0;
}
