#ifndef LOW_LATENCY_TRADING_APP_FIFO_SEQUENCER_H
#define LOW_LATENCY_TRADING_APP_FIFO_SEQUENCER_H

#include "../exchange/order_server_request.h"
#include "../exchange/types.h"
#include "lib/assertion.h"
#include "lib/logger.h"
#include "lib/time_utils.h"
#include <algorithm>
#include <array>

namespace Exchange {

/**
 * @class FIFOSequencer
 * @brief First-in, first-out queue for sequencing order events.
 *
 * This component ensures client order request packets are processed in the order
 * they're received, irrespective of any TCP multiplexing latencies. It handles
 * forwarding order requests from the gateway to the exchange matching engine.
 */
class FIFOSequencer {
  public:
    /**
     * @brief Construct a new FIFOSequencer
     * @param rxRequests Reference to the ClientRequestQueue for receiving requests
     */
    explicit FIFOSequencer(ClientRequestQueue& rxRequests) noexcept : rxRequests_(rxRequests) {}

    FIFOSequencer() = delete;
    FIFOSequencer(const FIFOSequencer&) = delete;
    FIFOSequencer& operator=(const FIFOSequencer&) = delete;
    FIFOSequencer(FIFOSequencer&&) noexcept = delete;
    FIFOSequencer& operator=(FIFOSequencer&&) noexcept = delete;

    /**
     * @brief Sequences and publishes all queued order requests to the Order Matching Engine.
     */
    void sequenceAndPublish() noexcept {
        if (nPendingRequests_ == 0) [[unlikely]] {
            return;
        }

        // Sort pending requests by their timestamps using a custom comparator
        std::sort(pendingRequests_.begin(),
                  pendingRequests_.begin() + nPendingRequests_,
                  [](const PendingClientRequest& a, const PendingClientRequest& b) { return a.tRx < b.tRx; });

        for (size_t i = 0; i < nPendingRequests_; ++i) {
            const auto& req = pendingRequests_[i];
            LOG_INFO("Sequencing request: {} at tRx: {}", req.request.toStr(), req.tRx);
            rxRequests_.push(req.request);
        }

        nPendingRequests_ = 0;
    }

    /**
     * @brief Push a pending client order request onto the queue
     * @param request The client order request
     * @param tRx The reception timestamp
     */
    void pushClientRequest(const OMEClientRequest& request, utils::Nanos tRx) noexcept {
        if (nPendingRequests_ >= pendingRequests_.size()) [[unlikely]] {
            FATAL("<FIFOSequencer> Too many pending requests!");
        }
        pendingRequests_[nPendingRequests_] = {tRx, request};
        nPendingRequests_++;
    }

  private:
    /**
     * @struct PendingClientRequest
     * @brief Tracks a client request which is awaiting processing by the order gateway
     */
    struct PendingClientRequest {
        utils::Nanos tRx;
        OMEClientRequest request;
    };

    ClientRequestQueue& rxRequests_;
    std::array<PendingClientRequest, Exchange::Types::MAX_PENDING_ORDER_REQUESTS> pendingRequests_{};
    size_t nPendingRequests_{0};
};

} // namespace Exchange

#endif // LOW_LATENCY_TRADING_APP_FIFO_SEQUENCER_H