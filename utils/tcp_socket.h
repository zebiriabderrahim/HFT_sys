//
// Created by ABDERRAHIM ZEBIRI on 2024-08-23.
//

#ifndef LOW_LATENCY_TRADING_APP_TCP_SOCKET_H
#define LOW_LATENCY_TRADING_APP_TCP_SOCKET_H

#include <functional>
#include <string>
#include <vector>
#include <span>
#include "socket_utils.h"

namespace utils {

constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

class TCPSocket {
  public:
    TCPSocket() noexcept;
    ~TCPSocket() noexcept;

    TCPSocket(const TCPSocket&) = delete;
    TCPSocket(TCPSocket&&) = delete;
    TCPSocket& operator=(const TCPSocket&) = delete;
    TCPSocket& operator=(TCPSocket&&) = delete;

    [[nodiscard]] auto connect(std::string_view ip, std::string_view iface, int port, bool isListening) -> int;
    [[nodiscard]] auto sendAndRecv() noexcept -> bool;
    auto send(std::span<const std::byte> data) noexcept -> void;

    void setRecvCallback(std::function<void(TCPSocket*, Nanos)> callback) noexcept {
        recvCallback_ = std::move(callback);
    }

  private:
    int socketFd_{-1};
    std::vector<std::byte> outboundData_;
    std::vector<std::byte> inboundData_;
    size_t nextSendValidIndex_{0};
    size_t nextRcvValidIndex_{0};
    sockaddr_in socketAttrib_{};
    std::function<void(TCPSocket*, Nanos)> recvCallback_{nullptr};
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_TCP_SOCKET_H
