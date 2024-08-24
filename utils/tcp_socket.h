/**
* @file tcp_socket.h
* @brief Defines the TCPSocket class for low-latency network communication.
* @author ABDERRAHIM ZEBIRI
* @date 2024-08-23
*/

#ifndef LOW_LATENCY_TRADING_APP_TCP_SOCKET_H
#define LOW_LATENCY_TRADING_APP_TCP_SOCKET_H

#include <functional>
#include <string>
#include <vector>
#include <span>
#include "socket_utils.h"

namespace utils {

/// Size of send and receive buffers in bytes.
constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

/**
* @class TCPSocket
* @brief Represents a TCP socket for low-latency network communication.
*
* This class provides functionality for creating, connecting, and managing TCP sockets
* with a focus on low-latency operations. It supports non-blocking I/O and timestamp options.
*/
class TCPSocket {
 public:
   /**
    * @brief Constructs a TCPSocket object.
    *
    * Initializes the send and receive buffers to TCPBufferSize.
    */
   TCPSocket() noexcept;

   /**
    * @brief Destroys the TCPSocket object.
    *
    * Closes the socket if it's open.
    */
   ~TCPSocket() noexcept;

   // Delete copy and move operations
   TCPSocket(const TCPSocket&) = delete;
   TCPSocket(TCPSocket&&) = delete;
   TCPSocket& operator=(const TCPSocket&) = delete;
   TCPSocket& operator=(TCPSocket&&) = delete;

   /**
    * @brief Connects the socket to a specified address.
    *
    * @param ip IP address to connect to.
    * @param iface Network interface to use.
    * @param port Port number to connect to.
    * @param isListening If true, sets up the socket for listening instead of connecting.
    * @return The file descriptor of the connected socket.
    */
   [[nodiscard]] auto connect(std::string_view ip, std::string_view iface, int port, bool isListening) -> int;

   /**
    * @brief Performs non-blocking send and receive operations.
    *
    * @return true if data was received, false otherwise.
    */
   [[nodiscard]] auto sendAndRecv() noexcept -> bool;

   /**
    * @brief Sends data through the socket.
    *
    * @param data Span of bytes to send.
    */
   auto send(std::span<const std::byte> data) noexcept -> void;

   /**
    * @brief Sets the callback function for receive events.
    *
    * @param callback Function to be called when data is received.
    */
   void setRecvCallback(std::function<void(TCPSocket*, Nanos)> callback) noexcept {
       recvCallback_ = std::move(callback);
   }

 private:
   int socketFd_{-1};  ///< File descriptor for the socket.
   std::vector<std::byte> outboundData_;  ///< Buffer for outgoing data.
   std::vector<std::byte> inboundData_;   ///< Buffer for incoming data.
   size_t nextSendValidIndex_{0};  ///< Next valid index for sending data.
   size_t nextRcvValidIndex_{0};   ///< Next valid index for receiving data.
   sockaddr_in socketAttrib_{};    ///< Socket attributes.
   std::function<void(TCPSocket*, Nanos)> recvCallback_{nullptr};  ///< Callback for receive events.
};

} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_TCP_SOCKET_H