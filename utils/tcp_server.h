//
// Created by ABDERRAHIM ZEBIRI on 2024-08-24.
//
#ifndef LOW_LATENCY_TRADING_APP_TCP_SERVER_H
#define LOW_LATENCY_TRADING_APP_TCP_SERVER_H

#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif

#include "tcp_socket.h"
#include "memory_pool.h"


namespace utils {

using RecvCallback = std::function<void(TCPSocket *, Nanos)>;

/**
 * @class TCPServer
 * @brief A TCP server implementation for low-latency applications.
 *
 * This class provides functionality to create and manage a TCP server,
 * handling multiple client connections efficiently using epoll (on Linux)
 * or kqueue (on macOS/BSD systems).
 */
class TCPServer {
  public:

    TCPServer() noexcept = default;

    /**
     * @brief Destructor. Ensures proper cleanup of resources.
     */
    ~TCPServer() noexcept;

    // Delete copy and move operations to prevent unintended resource sharing
    TCPServer(const TCPServer &) = delete;
    TCPServer &operator=(const TCPServer &) = delete;
    TCPServer(TCPServer &&) = delete;
    TCPServer &operator=(TCPServer &&) = delete;

    /**
     * @brief Stops the server and cleans up resources.
     */
    auto stop() noexcept -> void;

    /**
     * @brief Starts listening for incoming connections on the specified interface and port.
     * @param interfaceName The network interface to listen on.
     * @param port The port number to listen on.
     */
    auto listen(std::string_view interfaceName, int port) -> void;

    /**
     * @brief Polls for events on the server and client sockets.
     *
     * This method checks for new connections, readable/writable sockets,
     * and handles them accordingly.
     */
    auto poll() noexcept -> void;

    /**
     * @brief Sends and receives data on all connected sockets.
     *
     * This method processes all pending send and receive operations
     * on the connected client sockets.
     */
    auto sendAndReceive() noexcept -> void;

    /**
     * @brief Sets the callback function to be called when data is received.
     * @param callback The function to be called.
     */
    auto setRecvCallback(RecvCallback callback) noexcept -> void;

    /**
     * @brief Sets the callback function to be called when all receive operations are finished.
     * @param callback The function to be called.
     */
    auto setRecvFinishedCallback(std::function<void()> callback) noexcept -> void;

  private:
    /**
     * @brief Adds a socket to the event monitoring system (epoll or kqueue).
     * @param socket The socket to be added.
     * @return True if the socket was successfully added, false otherwise.
     */
    [[nodiscard]] auto addSocketToEventSystem(TCPSocket *socket) const noexcept -> bool;

    static constexpr size_t MAX_EVENTS = 1024;

    int eventFd_{-1};
#ifdef __linux__
    epoll_event events_[MAX_EVENTS];
#else
    struct kevent events_[MAX_EVENTS]{};
#endif

    TCPSocket listenerSocket_;
    std::vector<TCPSocket *> receiveSockets_;
    std::vector<TCPSocket *> sendSockets_;
    MemoryPool<TCPSocket> socketPool_{MAX_EVENTS};

    RecvCallback recvCallback_ = nullptr;
    std::function<void()> recvFinishedCallback_ = nullptr;
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_TCP_SERVER_H