//
// Created by ABDERRAHIM ZEBIRI on 2024-09-15.
//

#ifndef LOW_LATENCY_TRADING_APP_ORDER_BOOK_H
#define LOW_LATENCY_TRADING_APP_ORDER_BOOK_H

#include <array>
#include <memory>
#include <string>
#include <string_view>

#include "../exchange/market_data.h"
#include "../exchange/order_server_request.h"
#include "../exchange/order_server_response.h"
#include "../exchange/types.h"
#include "../exchange/matching_engine_order.h"
#include "lib/lock_free_queue.h"
#include "lib/logger.h"
#include "lib/memory_pool.h"

namespace MatchingEngine {

class MatchingEngine;

/**
 * @class OrderBook
 * @brief Represents a limit order book for a single financial instrument.
 *
 * This class manages the orders for a specific ticker, handling order additions,
 * cancellations, and matches. It's designed for high-performance in a low-latency
 * trading environment.
 */
class OrderBook {
  public:
    /**
     * @brief Constructs a limit order book for a single financial instrument/ticker.
     * @param assignedTicker Financial instrument ID
     * @param ome Reference to the parent Order Matching Engine
     */
    explicit OrderBook(Exchange::TickerID assignedTicker, MatchingEngine& ome) noexcept;

    ~OrderBook();

    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) noexcept = delete;
    OrderBook& operator=(OrderBook&&) noexcept = delete;

    /**
     * @brief Adds a new entry into the limit order book.
     * @details Matches with existing orders and places the remainder into the book.
     *          Generates an OMEClientResponse and attempts to match the order.
     */
    void addOrder(Exchange::ClientID clientId, Exchange::OrderID clientOid, Exchange::TickerID tickerId,
                  Exchange::Side side, Exchange::Price price, Exchange::Qty qty) noexcept;

    /**
     * @brief Cancels an existing order in the book, if possible.
     */
    void cancelOrder(Exchange::ClientID clientId, Exchange::OrderID orderId, Exchange::TickerID tickerId) noexcept;

    /**
     * @brief Returns a string representation of the order book contents.
     * @param isDetailed If true, includes more detailed information
     * @param hasValidityCheck If true, performs additional validity checks
     * @return A string representation of the order book
     */
    [[nodiscard]] std::string toString(bool isDetailed, bool hasValidityCheck) const;

  private:
    Exchange::TickerID assignedTicker_{Exchange::TickerID_INVALID};
    MatchingEngine& ome_;

    Exchange::ClientOrderMap mapClientIdToOrder_{};
    Exchange::OrdersAtPrice* bidsByPrice_{nullptr};
    Exchange::OrdersAtPrice* asksByPrice_{nullptr};
    Exchange::OrdersAtPriceMap mapPriceToPriceLevel_{};
    utils::MemoryPool<Exchange::OrdersAtPrice> ordersAtPricePool_{Exchange::Types::MAX_PRICE_LEVELS};
    utils::MemoryPool<Exchange::Order> orderPool_{Exchange::Types::MAX_ORDER_IDS};

    Exchange::OMEClientResponse clientResponse_;
    Exchange::OMEMarketUpdate marketUpdate_;
    Exchange::OrderID nextMarketOid_{1};

    [[nodiscard]] Exchange::Qty findMatch(Exchange::ClientID clientId, Exchange::OrderID clientOid,
                                          Exchange::TickerID tickerId, Exchange::Side side, Exchange::Price price,
                                          Exchange::Qty qty, Exchange::OrderID newMarketOid) noexcept;

    void matchOrder(Exchange::TickerID tickerId, Exchange::ClientID clientId, Exchange::Side side,
                    Exchange::OrderID clientOrderId, Exchange::OrderID newMarketOid,
                    Exchange::Order* orderMatched, Exchange::Qty* qtyRemains) noexcept;

    [[nodiscard]] inline Exchange::OrderID getNewMarketOrderId() noexcept {
        return nextMarketOid_++;
    }

    void addPriceLevel(Exchange::OrdersAtPrice* newOrdersAtPrice) noexcept;
    void removePriceLevel(Exchange::Side side, Exchange::Price price) noexcept;

    [[nodiscard]] inline Exchange::Priority getNextPriority(Exchange::Price price) const noexcept {
        const auto ordersAtPrice = getLevelForPrice(price);
        return ordersAtPrice ? ordersAtPrice->order0_->prev_->priority_ + 1ul : 1ul;
    }

    [[nodiscard]] static constexpr std::size_t priceToIndex(Exchange::Price price) noexcept {
        return static_cast<std::size_t>(price % Exchange::Types::MAX_PRICE_LEVELS);
    }

    [[nodiscard]] inline Exchange::OrdersAtPrice* getLevelForPrice(Exchange::Price price) const noexcept {
        return mapPriceToPriceLevel_[priceToIndex(price)];
    }

    void addOrderToBook(Exchange::Order* order) noexcept;
    void removeOrderFromBook( Exchange::Order* order) noexcept;
};

/**
 * @typedef OrderBookMap
 * @brief Mapping of tickers to their limit order books
 */
using OrderBookMap = std::array<std::unique_ptr<OrderBook>, Exchange::Types::MAX_TICKERS>;

} // namespace MatchingEngine

#endif // LOW_LATENCY_TRADING_APP_ORDER_BOOK_H
