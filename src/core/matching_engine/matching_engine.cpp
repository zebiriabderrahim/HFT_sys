#include "matching_engine.h"

#include "lib/debug_assertion.h"
#include "lib/thread_util.h"
#include "lib/logger.h"
#include "lib/time_utils.h"

namespace MatchingEngine {

MatchingEngine::MatchingEngine(Exchange::ClientRequestQueue& rxRequests,
                               Exchange::ClientResponseQueue& txResponses,
                               Exchange::MarketUpdateQueue& txMarketUpdates) noexcept
    : rxRequests_(rxRequests),
      txResponses_(txResponses),
      txMarketUpdates_(txMarketUpdates) {
    orderBookForTicker_.fill(nullptr);
    for (size_t i = 0; i < orderBookForTicker_.size(); ++i) {
        orderBookForTicker_[i] = std::make_unique<OrderBook>(static_cast<Exchange::TickerID>(i), *this);
    }
}

MatchingEngine::~MatchingEngine() {
    stopMatchingEngine();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
}

void MatchingEngine::startMatchingEngine() noexcept {
    matchingEngineThread_ = utils::createAndStartThread(-1, "OME", [this]() { runMatchingEngine(); });
    ASSERT_CONDITION(matchingEngineThread_ != nullptr, "MatchingEngine Failed to start thread for matching engine");
}

void MatchingEngine::stopMatchingEngine() noexcept {
    if (matchingEngineThread_ && matchingEngineThread_->joinable()) {
        matchingEngineThread_->request_stop();
        matchingEngineThread_->join();
    }
}

void MatchingEngine::handleClientRequest(const Exchange::OMEClientRequest& request) noexcept {
    switch (request.type) {
        using namespace Exchange;

    case OMEClientRequest::Type::NEW:
        orderBookForTicker_[request.tickerId]->addOrder(
            request.clientId, request.orderId,
            request.tickerId, request.side,
            request.price, request.qty
        );
        break;
    case OMEClientRequest::Type::CANCEL:
        orderBookForTicker_[request.tickerId]->cancelOrder(
            request.clientId, request.orderId,
            request.tickerId);
        break;
    default:
        LOG_INFO("Received invalid client request: {}", OMEClientRequest::typeToStr(request.type));
        break;
    }
}

void MatchingEngine::dispatchClientResponse(const Exchange::OMEClientResponse& response) noexcept {
    LOG_INFO("Publishing market update: {}", response.toStr());
    if (!txResponses_.push(response)) [[unlikely]] {
        LOG_ERROR("Failed to push client response to queue");
    }
}

void MatchingEngine::publishMarketUpdate(const Exchange::OMEMarketUpdate& update) noexcept {
    LOG_INFO("Publishing market update: {}", update.toStr());
    if (!txMarketUpdates_.push(update)) [[unlikely]] {
        LOG_ERROR("Failed to push market update to queue");
    }
}

void MatchingEngine::runMatchingEngine() noexcept {
    isRunning_.store(true, std::memory_order_relaxed);
    LOG_INFO("Matching engine thread started");
    while (isRunning_.load(std::memory_order_relaxed)) {
        if (auto request = rxRequests_.pop()) [[likely]] {
            LOG_INFO("rx request: {} {}", utils::getCurrentTimeStr(), request->toStr());
            handleClientRequest(*request);
        }
    }
    isRunning_.store(false, std::memory_order_relaxed);
}

} // namespace MatchingEngine