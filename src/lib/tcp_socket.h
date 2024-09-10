/**
 * @file tcp_socket.h
 * @brief Defines the TCPSocket class for both server and client in low-latency network communication.
 * @author ABDERRAHIM ZEBIRI
 * @date 2024-08-23
 */

#ifndef LOW_LATENCY_TRADING_APP_TCP_SOCKET_H
#define LOW_LATENCY_TRADING_APP_TCP_SOCKET_H

#include "socket_utils.h"
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace utils {

/// Size of send and receive buffers in bytes.
constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

/**
 * @class TCPSocket
 * @brief Represents a TCP socket for low-latency network communication in trading applications.
 *
 * This class builds upon the socket utilities to provide a high-level interface for TCP communication.
 * It is designed for use in both server and client contexts within low-latency trading systems.
 * Key features include:
 * - Support for both sending and receiving data
 * - Non-blocking I/O operations
 * - Large internal buffers for high-throughput scenarios
 * - Timestamp support for precise timing measurements
 * - Callback mechanism for handling received data
 */
class TCPSocket {
  public:
    /**
     * @brief Constructs a TCPSocket object.
     *
     * Initializes the send and receive buffers to TCPBufferSize.
     */
    TCPSocket() noexcept;

    // Delete copy and move operations
    TCPSocket(const TCPSocket &) = delete;
    TCPSocket(TCPSocket &&) = delete;
    TCPSocket &operator=(const TCPSocket &) = delete;
    TCPSocket &operator=(TCPSocket &&) = delete;

    /**
     * @brief Gets the file descriptor of the socket.
     *
     * @return The file descriptor of the socket.
     */
    [[nodiscard]] auto getSocketFd() const noexcept -> int { return socketFd_; }

    /**
     * @brief Gets the inbound and outbound data buffers.
     *
     * @return The inbound and outbound data buffers.
     */
    [[nodiscard]] auto getInboundData() const noexcept -> const std::vector<char> & { return inboundData_; }

    /**
     * @brief Gets the outbound data buffer.
     *
     * @return The outbound data buffer.
     */
    [[nodiscard]] auto getOutboundData() const noexcept -> const std::vector<char> & { return outboundData_; }

    /**
     * @brief Gets the next valid index for sending data.
     *
     * @return The next valid index for sending data.
     */
    [[nodiscard]] auto getNextSendValidIndex() const noexcept -> size_t { return nextSendValidIndex_; }

    /**
     * @brief Gets the next valid index for receiving data.
     *
     * @return The next valid index for receiving data.
     */
    [[nodiscard]] auto getNextRcvValidIndex() const noexcept -> size_t { return nextRcvValidIndex_; }

    /**
     * @brief Sets the file descriptor of the socket.
     *
     * @param socketFd File descriptor of the socket.
     */
    auto setSocketFd(int socketFd) noexcept -> void { socketFd_ = socketFd; }

    /**
     * @brief Resets the inbound data buffer.
     */
    auto restNextSendValidIndex() noexcept -> void { nextSendValidIndex_ = 0; }

    /**
     * @brief Resets the next valid index for sending data.
     */
    auto restNextRcvValidIndex() noexcept -> void { nextRcvValidIndex_ = 0; }

    /**
     * @brief Connects the socket to a specified address.
     *
     * @param ip IP address to connect to.
     * @param iface Network interface to use.
     * @param port Port number to connect to.
     * @param isListening If true, sets up the socket for listening instead of connecting.
     * @return The file descriptor of the connected socket.
     */
    auto connect(std::string_view ip, std::string_view iface, int port, bool isListening) -> int;

    /**
     * @brief Performs non-blocking send and receive operations.
     *
     * @return true if data was received, false otherwise.
     */
    auto sendAndRecv() noexcept -> bool;

    /**
     * @brief Sends data through the socket.
     *
     * @param data Span of bytes to send.
     */
    auto send(const void *data, size_t len) noexcept -> void;

    /**
     * @brief Sets the callback function for receive events.
     *
     * @param callback Function to be called when data is received.
     */
    void setRecvCallback(std::function<void(TCPSocket *, Nanos)> callback) noexcept { recvCallback_ = std::move(callback); }

  private:
    int socketFd_{-1};                                              ///< File descriptor for the socket.
    std::vector<char> outboundData_;                                ///< Buffer for outgoing data.
    std::vector<char> inboundData_;                                 ///< Buffer for incoming data.
    size_t nextSendValidIndex_{0};                                  ///< Next valid index for sending data.
    size_t nextRcvValidIndex_{0};                                   ///< Next valid index for receiving data.
    sockaddr_in socketAttrib_{};                                    ///< Socket attributes.
    std::function<void(TCPSocket *, Nanos)> recvCallback_{nullptr}; ///< Callback for receive events.
};

} // namespace lib

#endif // LOW_LATENCY_TRADING_APP_TCP_SOCKET_H