#include "router/failure_detector.h"

#include <exception>
#include <optional>
#include <utility>

#include "router/address.h"

namespace shardcache {

FailureDetector::FailureDetector(std::vector<std::string> nodes,
                                 std::chrono::milliseconds interval, int misses_to_fail)
    : nodes_(std::move(nodes)), interval_(interval), misses_to_fail_(misses_to_fail) {}

FailureDetector::~FailureDetector() {
    {
        std::lock_guard<std::mutex> lock(stop_mutex_);
        stopping_ = true;
    }
    stop_cv_.notify_all();  
    if (thread_.joinable()) {
        thread_.join();
    }
}

void FailureDetector::start() {
    thread_ = std::thread([this] { run(); });
}

bool FailureDetector::is_alive(const std::string& node) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !down_.contains(node);
}

void FailureDetector::run() {
    while (true) {
        check_once();
        std::unique_lock<std::mutex> lock(stop_mutex_);
        if (stop_cv_.wait_for(lock, interval_, [this] { return stopping_; })) {
            return;
        }
    }
}

void FailureDetector::check_once() {
    for (const std::string& node : nodes_) {
        if (!is_alive(node)) {
            continue; 
        }
        if (ping(node)) {
            misses_[node] = 0;
            continue;
        }
        ++misses_[node];
        if (misses_[node] >= misses_to_fail_) {
            std::lock_guard<std::mutex> lock(mutex_);
            down_.insert(node);
        }
    }
}

bool FailureDetector::ping(const std::string& node) {
    try {
        auto it = connections_.find(node);
        if (it == connections_.end()) {
            std::pair<std::string, std::uint16_t> address = split_address(node);
            net::Socket socket = net::Socket::connect_to(address.first, address.second);
            socket.set_receive_timeout(interval_);
            it = connections_.emplace(node, std::make_unique<Connection>(std::move(socket))).first;
        }
        Connection& connection = *it->second;
        connection.socket.send_all("PING\n");
        std::optional<std::string> reply = connection.reader.read_line();
        if (reply == "PONG") {
            return true;
        }
    } catch (const std::exception&) {
        //connect refused, send failed, or no PONG in time - all count as a missed heartbeat
    }
    connections_.erase(node);
    return false;
}

}
