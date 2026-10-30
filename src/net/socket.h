#pragma once

#include <chrono>
#include <cstdint>
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

    static Socket connect_to(const std::string& host, std::uint16_t port);

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
