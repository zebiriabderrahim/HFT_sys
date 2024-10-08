//
// Created by ABDERRAHIM ZEBIRI on 2024-09-15.
//

#ifndef LOW_LATENCY_TRADING_APP_MATCHING_ENGINE_H
#define LOW_LATENCY_TRADING_APP_MATCHING_ENGINE_H


#include <thread>
#include <memory>
#include <atomic>

#include "../exchange/market_data.h"
#include "../exchange/order_server_request.h"
#include "../exchange/order_server_response.h"
#include "../exchange/types.h"
#include "lib/lock_free_queue.h"
#include "lib/logger.h"

#include "order_book.h"

namespace MatchingEngine {

/**
 * @class MatchingEngine
 * @brief Primary exchange component for matching bid and ask orders from market participants.
 *
 * Runs on a dedicated thread, maintaining order books for each supported instrument.
 * Receives and responds to client orders via the Order Gateway, and publishes data
 * by dispatching to the market data publisher.
 */
class MatchingEngine {
  public:
    /**
     * @brief Constructs a MatchingEngine instance.
     * @param rxRequests Queue for receiving client order requests.
     * @param txResponses Queue for transmitting responses to client orders.
     * @param txMarketUpdates Queue for pushing market updates to the publisher.
     */
    MatchingEngine(Exchange::ClientRequestQueue& rxRequests,
                   Exchange::ClientResponseQueue& txResponses,
                   Exchange::MarketUpdateQueue& txMarketUpdates) noexcept;

    // Rule of five
    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;
    MatchingEngine(MatchingEngine&&) noexcept = delete;
    MatchingEngine& operator=(MatchingEngine&&) noexcept = delete;
    ~MatchingEngine();

    /**
     * @brief Starts the matching thread.
     */
    void startMatchingEngine() noexcept;

    /**
     * @brief Stops the matching thread.
     */
    void stopMatchingEngine() noexcept;

    /**
     * @brief Processes a client request received from the order gateway server.
     * @param request The client request to process.
     */
    void handleClientRequest(const Exchange::OMEClientRequest& request) noexcept;

    /**
     * @brief Dispatches a response to a client via the order gateway server.
     * @param response The response to send to the client.
     */
    void dispatchClientResponse(const Exchange::OMEClientResponse& response) noexcept;

    /**
     * @brief Dispatches a market update to the market data publisher.
     * @param update The market update to dispatch.
     */
    void publishMarketUpdate(const Exchange::OMEMarketUpdate& update) noexcept;

    /**
     * @brief Checks if the matching engine worker thread is running.
     * @return True if the thread is running, false otherwise.
     */
    [[nodiscard]] bool isMatchingEngineRunning() const noexcept {
        return isRunning_.load(std::memory_order_relaxed);
    }

  private:
    /**
     * @brief Runs the main matching engine loop.
     * Processes client requests received from the rxRequests_ queue.
     */
    void runMatchingEngine() noexcept;

    OrderBookMap orderBookForTicker_;
    Exchange::ClientRequestQueue& rxRequests_;
    Exchange::ClientResponseQueue& txResponses_;
    Exchange::MarketUpdateQueue& txMarketUpdates_;
    std::unique_ptr<std::jthread> matchingEngineThread_{nullptr};
    std::atomic<bool> isRunning_{false};
};

} // namespace MatchingEngine

#endif // LOW_LATENCY_TRADING_APP_MATCHING_ENGINE_H