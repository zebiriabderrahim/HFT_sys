//
// Created by ABDERRAHIM ZEBIRI on 2024-08-24.
//
#ifndef LOW_LATENCY_TRADING_APP_TCP_SERVER_H
#define LOW_LATENCY_TRADING_APP_TCP_SERVER_H

#include <functional>
#include <string_view>
#include <vector>
#include <memory>
#include <span>

#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif

#include "tcp_socket.h"

namespace utils {

using RecvCallback = std::function<void(TCPSocket *, Nanos)>;

class TCPServer {
  public:
    TCPServer() = default;

    void stop() noexcept {
        if (eventFd_ != -1) {
            close(eventFd_);
            eventFd_ = -1;
        }

        receiveSockets_.clear();
        sendSockets_.clear();
    }
    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;
    TCPServer(TCPServer&&) = delete;
    TCPServer& operator=(TCPServer&&) = delete;

    auto listen(std::string_view interfaceName, int port) -> void;
    auto poll() noexcept -> void;
    auto sendAndReceive() noexcept -> void;
    void setRecvCallback(RecvCallback callback) noexcept {recvCallback_ = std::move(callback);}
    void setRecvFinishedCallback(std::function<void()> callback) { recvFinishedCallback_ = std::move(callback);}

  private:
    [[nodiscard]] auto addSocketToEventSystem(const std::shared_ptr<TCPSocket>& socket) const noexcept -> bool;

    static constexpr size_t MAX_EVENTS = 1024;

    int eventFd_{-1};
#ifdef __linux__
    epoll_event events_[MAX_EVENTS];
#else
    struct kevent events_[MAX_EVENTS]{};
#endif

    std::shared_ptr<TCPSocket> listenerSocket_;
    std::vector<std::shared_ptr<TCPSocket>> receiveSockets_;
    std::vector<std::shared_ptr<TCPSocket>> sendSockets_;

    RecvCallback recvCallback_ = nullptr;
    std::function<void()> recvFinishedCallback_ = nullptr;
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_TCP_SERVER_H