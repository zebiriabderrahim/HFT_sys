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

class TCPServer {
  public:
    TCPServer() = default;
    ~TCPServer();

    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;
    TCPServer(TCPServer&&) = delete;
    TCPServer& operator=(TCPServer&&) = delete;

    [[nodiscard]] auto listen(std::string_view interfaceName, int port) -> bool;
    auto poll() noexcept -> void;
    auto sendAndReceive() noexcept -> void;
    auto stop() noexcept -> void;

    void setRecvCallback(std::function<void(TCPSocket*, Nanos)> callback) { recvCallback_ = std::move(callback);}
    void setRecvFinishedCallback(std::function<void()> callback) { recvFinishedCallback_ = std::move(callback);}

  private:
    auto addSocketToEventSystem(TCPSocket* socket) const noexcept -> bool;

    static constexpr size_t MAX_EVENTS = 1024;

    int eventFd_{-1};
    std::unique_ptr<TCPSocket> listenerSocket_;
#ifdef __linux__
    epoll_event events_[MAX_EVENTS];
    std::span<epoll_event> events_span_{events_, MAX_EVENTS};
#else
    struct kevent events_[MAX_EVENTS];
    std::span<struct kevent> events_span_{events_, MAX_EVENTS};
#endif

    std::vector<std::unique_ptr<TCPSocket>> receiveSockets_;
    std::vector<std::unique_ptr<TCPSocket>> sendSockets_;

    std::function<void(TCPSocket*, Nanos)> recvCallback_ = nullptr;
    std::function<void()> recvFinishedCallback_ = nullptr;
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_TCP_SERVER_H