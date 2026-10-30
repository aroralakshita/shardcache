#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#include "net/socket.h"

#include <algorithm>
#include <climits>
#include <stdexcept>
#include <string>
#include <utility>

namespace shardcache::net {

namespace {

static_assert(sizeof(SOCKET) == sizeof(std::uintptr_t));
static_assert(INVALID_SOCKET == static_cast<SOCKET>(~std::uintptr_t{0}));

struct WinsockRuntime {
    WinsockRuntime() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }
    ~WinsockRuntime() { WSACleanup(); }
};

void ensure_winsock() {
    static WinsockRuntime runtime;
}

[[noreturn]] void throw_last_error(const char* operation) {
    throw std::runtime_error(std::string(operation) + " failed (WSA error " +
                             std::to_string(WSAGetLastError()) + ")");
}

SOCKET os_handle(std::uintptr_t handle) {
    return static_cast<SOCKET>(handle);
}

}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : handle_(std::exchange(other.handle_, kInvalid)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = std::exchange(other.handle_, kInvalid);
    }
    return *this;
}

Socket Socket::listen_on(std::uint16_t port) {
    ensure_winsock();
    Socket s(static_cast<std::uintptr_t>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)));
    if (!s.valid()) {
        throw_last_error("socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);                    
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); 
    if (::bind(os_handle(s.handle_), reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) ==
        SOCKET_ERROR) {
        throw_last_error("bind");
    }
    if (::listen(os_handle(s.handle_), SOMAXCONN) == SOCKET_ERROR) {
        throw_last_error("listen");
    }
    return s;
}

Socket Socket::connect_to(const std::string& host, std::uint16_t port) {
    ensure_winsock();
    Socket s(static_cast<std::uintptr_t>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)));
    if (!s.valid()) {
        throw_last_error("socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        throw std::runtime_error("not an IPv4 address: " + host);
    }
    if (::connect(os_handle(s.handle_), reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) ==
        SOCKET_ERROR) {
        throw_last_error("connect");
    }
    return s;
}

Socket Socket::accept() const {
    SOCKET client = ::accept(os_handle(handle_), nullptr, nullptr);
    if (client == INVALID_SOCKET) {
        throw_last_error("accept");
    }
    return Socket(static_cast<std::uintptr_t>(client));
}

void Socket::send_all(std::string_view data) const {
    while (!data.empty()) {
        int chunk = static_cast<int>(std::min<std::size_t>(data.size(), INT_MAX));
        int sent = ::send(os_handle(handle_), data.data(), chunk, 0);
        if (sent == SOCKET_ERROR) {
            throw_last_error("send");
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

std::string Socket::receive_some() const {
    char buffer[4096];
    int received = ::recv(os_handle(handle_), buffer, static_cast<int>(sizeof(buffer)), 0);
    if (received == SOCKET_ERROR) {
        throw_last_error("recv");
    }
    return std::string(buffer, static_cast<std::size_t>(received));
}

void Socket::set_receive_timeout(std::chrono::milliseconds timeout) const {
    DWORD ms = static_cast<DWORD>(timeout.count()); 
    if (::setsockopt(os_handle(handle_), SOL_SOCKET, SO_RCVTIMEO,
                     reinterpret_cast<const char*>(&ms), sizeof(ms)) == SOCKET_ERROR) {
        throw_last_error("setsockopt(SO_RCVTIMEO)");
    }
}

std::uint16_t Socket::local_port() const {
    sockaddr_in addr{};
    int length = sizeof(addr);
    if (::getsockname(os_handle(handle_), reinterpret_cast<sockaddr*>(&addr), &length) ==
        SOCKET_ERROR) {
        throw_last_error("getsockname");
    }
    return ntohs(addr.sin_port);
}

bool Socket::valid() const {
    return handle_ != kInvalid;
}

void Socket::close() {
    if (valid()) {
        ::closesocket(os_handle(handle_));
        handle_ = kInvalid;
    }
}

}
