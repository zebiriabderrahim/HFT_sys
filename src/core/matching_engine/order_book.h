//
// Created by ABDERRAHIM ZEBIRI on 2024-09-15.
//

#ifndef LOW_LATENCY_TRADING_APP_ORDER_BOOK_H
#define LOW_LATENCY_TRADING_APP_ORDER_BOOK_H

#include "core/exchange/types.h"
#include "lib/memory_pool.h"
#include "lib/logger.h"

namespace MatchingEngine {

class OrderBook {

    class MatchingEngine;

      public:
        /**
     * @brief Constructs a limit order book for a single financial instrument/ticker.
     * @param assignedTicker Financial instrument ID
     * @param logger Logging instance to write to
     * @param ome Parent Order Matching Engine instance the book belongs to
         */
        explicit OrderBook(Exchange::TickerID assignedTicker,MatchingEngine& ome) noexcept;

        ~OrderBook();

        /**
     * @brief Adds a new entry into the limit order book.
     * @details Matches with existing orders and places the remainder into the book.
     * Generates an OMEClientResponse and attempts to match the order.
         */
        void addOrder(Exchange::ClientID clientId, Exchange::OrderID clientOid, Exchange::TickerID tickerId,
                      Exchange::Side side, Exchange::Price price, Exchange::Qty qty) noexcept;

        /**
     * @brief Cancels an existing order in the book, if possible.
         */
        void cancelOrder(Exchange::ClientID clientId, Exchange::OrderID orderId, Exchange::TickerID tickerId) noexcept;

        /**
     * @brief Returns a string representation of the order book contents.
         */
        [[nodiscard]] std::string toString(bool isDetailed, bool hasValidityCheck) const;

      private:
        Exchange::TickerID assignedTicker_{Exchange::TickerID_INVALID};
        MatchingEngine& ome_;

        ClientOrderMap mapClientIdToOrder_;
        OMEOrdersAtPrice* bidsByPrice_{nullptr};
        OMEOrdersAtPrice* asksByPrice_{nullptr};
        OrdersAtPriceMap mapPriceToPriceLevel_;
        utils::MemoryPool<Exchange::OMEOrdersAtPrice> ordersAtPricePool_{Exchange::Types::MAX_PRICE_LEVELS};
        utils::MemoryPool<Exchange::OMEOrder> orderPool_{Exchange::Types::MAX_ORDER_IDS};

        OMEClientResponse clientResponse_;
        OMEMarketUpdate marketUpdate_;
        OrderID nextMarketOid_{1};

        [[nodiscard]] Exchange::Qty findMatch(Exchange::ClientID clientId, Exchange::OrderID clientOid,
                                    Exchange::TickerID tickerId, Exchange::Side side, Exchange::Price price,
                                    Exchange::Qty qty, Exchange::OrderID newMarketOid) noexcept;

        void matchOrder(Exchange::TickerID tickerId, Exchange::ClientID clientId, Exchange::Side side,
                        Exchange::OrderID clientOrderId, Exchange::OrderID newMarketOid,
                        OMEOrder* orderMatched, Exchange::Qty* qtyRemains) noexcept;

        [[nodiscard]] inline Exchange::OrderID getNewMarketOrderId() noexcept {
            return nextMarketOid_++;
        }

        void addPriceLevel(OMEOrdersAtPrice* newOrdersAtPrice) noexcept;
        void removePriceLevel(Exchange::Side side, Exchange::Price price) noexcept;

        [[nodiscard]] inline Exchange::Priority getNextPriority(Exchange::Price price) const noexcept {
            const auto ordersAtPrice = getLevelForPrice(price);
            return ordersAtPrice ? ordersAtPrice->order_0->prev->priority + 1ul : 1ul;
        }

        [[nodiscard]] static constexpr size_t priceToIndex(Exchange::Price price) noexcept {
            return (price % Types::MAX_PRICE_LEVELS);
        }

        [[nodiscard]] inline OMEOrdersAtPrice* getLevelForPrice(Exchange::Price price) const noexcept {
            return mapPriceToPriceLevel[priceToIndex(price)];
        }

        void addOrderToBook(OMEOrder* order) noexcept;
        void removeOrderFromBook(OMEOrder* order) noexcept;
    };

    /**
 * @brief Mapping of tickers to their limit order book
     */
    using OrderBookMap = std::array<std::unique_ptr<OrderBook>, Exchange::Types::MAX_TICKERS>;

    } // namespace Exchange


};

} // namespace MatchingEngine

#endif // LOW_LATENCY_TRADING_APP_ORDER_BOOK_H
