//
// Created by ABDERRAHIM ZEBIRI on 2024-09-14.
//

#ifndef LOW_LATENCY_TRADING_APP_MATCHING_ENGINE_ORDER_H
#define LOW_LATENCY_TRADING_APP_MATCHING_ENGINE_ORDER_H

#include <array>
#include <string>
#include <format>

#include "types.h"

namespace Exchange {

/**
 * @class MatchingEngineOrder
 * @brief Represents a single order in the Order Matching Engine.
 *
 * This class encapsulates all the necessary information for an order
 * within the matching engine, including identifiers, pricing details,
 * and linked list pointers for efficient order book management.
 */
class MatchingEngineOrder {
  public:

    /**
     * @brief Struct for initializing OMEOrder
     *
     * This struct allows for aggregate initialization, which can be more
     * efficient and easier to read than passing multiple parameters.
     */
    struct MEOrder{
        TickerID tickerId;
        ClientID clientId;
        OrderID clientOrderId;
        OrderID marketOrderId;
        Side side;
        Price price;
        Qty qty;
        Priority priority;
        MatchingEngineOrder* prev;
        MatchingEngineOrder* next;
    };


    /**
     * @brief Default constructor
     */
    MatchingEngineOrder() = default;

        /**
         * @brief Constructs a MatchingEngineOrder with all its attributes
         *
         * @param order The order to copy attributes from
         */
    explicit MatchingEngineOrder(const MEOrder& order) noexcept
        : tickerId_(order.tickerId), clientId_(order.clientId), clientOrderId_(order.clientOrderId), marketOrderId_(order.marketOrderId),
          side_(order.side), price_(order.price), qty_(order.qty), priority_(order.priority), prev_(order.prev), next_(order.next) {
    }

    TickerID tickerId_{TickerID_INVALID};       ///< Financial instrument identifier
    ClientID clientId_{ClientID_INVALID};       ///< Market participant identifier
    OrderID clientOrderId_{OrderID_INVALID};   ///< Order ID provided by the client
    OrderID marketOrderId_{OrderID_INVALID};   ///< Unique market-wide order ID
    Side side_{Side::INVALID};                   ///< Buy or sell side_
    Price price_{Price_INVALID};                 ///< Ask or bid price_
    Qty qty_{Qty_INVALID};                       ///< Quantity still active in the order book
    Priority priority_{Priority_INVALID};        ///< Position in queue with respect to same price_ & side_

    MatchingEngineOrder* prev_{nullptr};  ///< Pointer to previous order at the same price_ level
    MatchingEngineOrder* next_{nullptr};  ///< Pointer to next_ order at the same price_ level

    /**
     * @brief Default comparison operator
     */
    bool operator==(const MatchingEngineOrder&) const = default;

    /**
     * @brief Converts the order to a string representation
     * @return A string containing all order details
     */
    [[nodiscard]] std::string to_string() const {
        return std::format("<MatchingEngineOrder>[ticker: {}, client: {}, oid_client: {}, oid_market: {}, "
                           "side_: {}, price_: {}, qty_: {}, priority_: {}, prev_: {}, next_: {}]",
                           tickerIdToStr(tickerId_), clientIdToStr(clientId_),
                           orderIdToStr(clientOrderId_), orderIdToStr(marketOrderId_),
                           sideToStr(side_), priceToStr(price_), qtyToStr(qty_),
                           priorityToStr(priority_),
                           orderIdToStr(prev_ ? prev_->marketOrderId_ : OrderID_INVALID),
                           orderIdToStr(next_ ? next_->marketOrderId_ : OrderID_INVALID));
    }
};

/**
 * @typedef OrderMap
 * @brief Mapping of OrderIDs to MatchingEngineOrder entries in the matching engine
 */
using OrderMap = std::array<MatchingEngineOrder*, Types::MAX_ORDER_IDS>;

/**
 * @typedef ClientOrderMap
 * @brief Mapping of client IDs to OrderMaps to MatchingEngineOrders
 */
using ClientOrderMap = std::array<OrderMap, Types::MAX_N_CLIENTS>;

/**
 * @class MatchingEngineOrdersAtPrice
 * @brief Represents all orders at a specific price_ level in the order book
 *
 * This class maintains a linked list of orders at the same price_ level,
 * facilitating efficient matching and order book management.
 */
class MatchingEngineOrdersAtPrice {
  public:
    /**
     * @brief Default constructor
     */
    MatchingEngineOrdersAtPrice() = default;

    /**
     * @brief Constructs a MatchingEngineOrdersAtPrice with all its attributes
     *
     * @param side Buy or sell side_
     * @param price The price_ level
     * @param order_0 Pointer to the first order at this price_ level
     * @param prev Pointer to the previously more aggressive price_ level
     * @param next Pointer to the next_ more aggressive price_ level
     */
    MatchingEngineOrdersAtPrice(Side side, Price price, MatchingEngineOrder* order_0,
                                MatchingEngineOrdersAtPrice* prev, MatchingEngineOrdersAtPrice* next) noexcept
        : side_(side), price_(price), order0_(order_0), prev_(prev), next_(next) {}

    /**
     * @brief Default comparison operator
     */
    bool operator==(const MatchingEngineOrdersAtPrice&) const = default;

    Side side_{Side::INVALID};                         ///< Buy or sell side_
    Price price_{Price_INVALID};                       ///< Price level
    MatchingEngineOrder* order0_{nullptr};             ///< Pointer to first order, sorted highest -> lowest priority_
    MatchingEngineOrdersAtPrice* prev_{nullptr};       ///< Pointer to previously more aggressive price_ level
    MatchingEngineOrdersAtPrice* next_{nullptr};       ///< Pointer to next_ more aggressive price_ level

    /**
     * @brief Converts the price_ level to a string representation
     * @return A string containing all price_ level details
     */
    [[nodiscard]] std::string to_string() const {
        return std::format("<MatchingEngineOrdersAtPrice>[side_: {}, price_: {}, order0_: {}, prev_: {}, next_: {}]",
                           sideToStr(side_), priceToStr(price_), order0_ ? order0_->to_string() : "NULL",
                           priceToStr(prev_ ? prev_->price_ : Price_INVALID),
                           priceToStr(next_ ? next_->price_ : Price_INVALID));
    }
};

/**
 * @typedef OrdersAtPriceMap
 * @brief Mapping of price_ levels to MatchingEngineOrdersAtPrice
 */
using OrdersAtPriceMap = std::array<MatchingEngineOrdersAtPrice*, Types::MAX_PRICE_LEVELS>;

} // namespace Exchange

#endif // LOW_LATENCY_TRADING_APP_MATCHING_ENGINE_ORDER_H
