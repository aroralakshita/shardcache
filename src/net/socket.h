#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace shardcache::net {

class Socket {
public:
    Socket() = default;  
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    static Socket listen_on(std::uint16_t port);

    //with a timeout, gives up after the specified duration instead of waiting for the
    //OS's connection attempt to finish
    static Socket connect_to(const std::string& host, std::uint16_t port,
                             std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    Socket accept() const;

    void send_all(std::string_view data) const;

    std::string receive_some() const;

    void set_receive_timeout(std::chrono::milliseconds timeout) const;

    std::uint16_t local_port() const;
    bool valid() const;
    void close();

private:
    explicit Socket(std::uintptr_t handle) : handle_(handle) {}

    static constexpr std::uintptr_t kInvalid = ~std::uintptr_t{0};
    std::uintptr_t handle_ = kInvalid;
};

}
