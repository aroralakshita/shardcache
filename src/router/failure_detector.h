#pragma once

#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "net/line_reader.h"
#include "net/socket.h"

namespace shardcache {

//heartbeat-based failure detector
class FailureDetector {
public:
    FailureDetector(std::vector<std::string> nodes, std::chrono::milliseconds interval,
                    int misses_to_fail);
    ~FailureDetector();  

    FailureDetector(const FailureDetector&) = delete;
    FailureDetector& operator=(const FailureDetector&) = delete;

    void start();

    bool is_alive(const std::string& node) const;

    void check_once();

private:
    struct Connection {
        explicit Connection(net::Socket s) : socket(std::move(s)), reader(socket) {}
        net::Socket socket;
        net::LineReader reader;
    };

    bool ping(const std::string& node);

    void run();

    std::vector<std::string> nodes_;
    std::chrono::milliseconds interval_;
    int misses_to_fail_;

    std::map<std::string, int> misses_;
    std::map<std::string, std::unique_ptr<Connection>> connections_;

    mutable std::mutex mutex_;
    std::set<std::string> down_;

    std::mutex stop_mutex_;
    std::condition_variable stop_cv_;
    bool stopping_ = false;

    std::thread thread_;
};

}
