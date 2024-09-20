#ifndef LOW_LATENCY_TRADING_APP_ORDER_GATEWAY_SERVER_H
#define LOW_LATENCY_TRADING_APP_ORDER_GATEWAY_SERVER_H

#include <string_view>
#include <memory>
#include <array>
#include <thread>
#include <atomic>

#include "core/exchange/order_server_request.h"
#include "core/exchange/order_server_response.h"
#include "fifo_Sequencer.h"
#include "lib/logger.h"
#include "lib/tcp_server.h"
#include "lib/tcp_socket.h"

namespace Exchange {

class OrderGatewayServer {
  public:
    /**
     * @brief Constructs an exchange order server which acts as a gateway
     * to process client order requests and respond with order messages from the matching engine.
     * @param txRequests Queue for passing order requests to the Order Matching Engine on behalf of participants
     * @param rxResponses Queue for receiving order responses from the matching engine
     * @param iface Network interface name to bind to
     * @param port Port the interface will listen on
     */
    OrderGatewayServer(ClientRequestQueue& txRequests,
                       ClientResponseQueue& rxResponses,
                       std::string_view iface, int port) noexcept;

    ~OrderGatewayServer();

    OrderGatewayServer(const OrderGatewayServer&) = delete;
    OrderGatewayServer& operator=(const OrderGatewayServer&) = delete;
    OrderGatewayServer(OrderGatewayServer&&) noexcept = default;
    OrderGatewayServer& operator=(OrderGatewayServer&&) noexcept = default;

    /**
     * @brief Starts the order server thread.
     */
    void start() noexcept;

    /**
     * @brief Stops the order server thread.
     */
    void stop() noexcept;

    /**
     * @brief Callback function for receiving data from a socket.
     * @param socket Pointer to the socket that received data
     * @param tRx Timestamp of data reception
     */
    void rxCallback(utils::TCPSocket* socket, utils::Nanos tRx) noexcept;

    /**
     * @brief Publishes all pending order requests to the exchange.
     */
    void rxDoneCallback() noexcept;

  private:
    /**
     * @brief The server thread's main working method.
     */
    void run() noexcept;

    const std::string iface_;
    const int port_;
    ClientResponseQueue& rxResponses_;
    std::atomic<bool> isRunning_{false};
    std::unique_ptr<std::jthread> serverThread_;

    utils::TCPServer server_;
    FIFOSequencer fifo_;
    std::array<std::size_t, Types::MAX_N_CLIENTS> mapClientToTxNSeq_{};
    std::array<std::size_t, Types::MAX_N_CLIENTS> mapClientToRxNSeq_{};
    std::array<utils::TCPSocket*, Types::MAX_N_CLIENTS> mapClientToSocket_{};
};

} // namespace Exchange

#endif // LOW_LATENCY_TRADING_APP_ORDER_GATEWAY_SERVER_H