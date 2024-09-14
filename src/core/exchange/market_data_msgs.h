//
// Created by ABDERRAHIM ZEBIRI on 2024-09-09.
//

#ifndef LOW_LATENCY_TRADING_APP_MARKET_DATA_MSGS_H
#define LOW_LATENCY_TRADING_APP_MARKET_DATA_MSGS_H

#include <string>
#include <format>
#include <cstdint>

#include "lib/lock_free_queue.h"
#include "types.h"

namespace Exchange {

/**
 * @brief Namespace containing message structures for market data dissemination
 * @details This namespace defines structures used for communication between
 * the Order Matching Engine (OME) and the Market Data Publisher (MDP)
 * in a high-frequency trading environment.
 */

/**
 * @brief Force 1-byte alignment for all structures in this scope
 * @details This is crucial for efficient network transmission and queue traversal
 * in high-frequency trading systems. It prioritizes compact data representation
 * over potential compiler speed optimizations.
 */

#pragma pack(push, 1) // 1-byte alignment for memory efficiency

/**
 * @brief Market update message sent from the Order Matching Engine (OME)
 * to the Market Data Publisher for broadcasting to clients.
 */
struct OMEMarketUpdate {
    enum class Type : std::uint8_t {
        INVALID = 0,
        CLEAR = 1,
        ADD = 2,
        MODIFY = 3,
        CANCEL = 4,
        TRADE = 5,
        SNAPSHOT_START = 6,
        SNAPSHOT_END = 7
    };

    Type type{Type::INVALID};             // Message type
    OrderID orderId{OrderID_INVALID};    // Order ID in the book
    TickerID tickerId{TickerID_INVALID}; // Ticker of the product
    Side side{Side::INVALID};             // Buy or sell
    Price price{Price_INVALID};           // Price of the order
    Qty qty{Qty_INVALID};                 // Quantity
    Priority priority{Priority_INVALID};  // Priority in the FIFO queue

    /**
     * @brief Converts the message type to a string representation.
     * @param type The message type to convert.
     * @return A string view representing the message type.
     */
    [[nodiscard]] static constexpr std::string_view typeToStr(Type type) noexcept {
        switch (type) {
            using enum Type;
        case CLEAR: return "CLEAR";
        case ADD: return "ADD";
        case MODIFY: return "MODIFY";
        case CANCEL: return "CANCEL";
        case TRADE: return "TRADE";
        case SNAPSHOT_START: return "SNAPSHOT_START";
        case SNAPSHOT_END: return "SNAPSHOT_END";
        case INVALID: return "INVALID";
        default: return "UNKNOWN";
        }
    }

    /**
     * @brief Converts the market update to a string representation.
     * @return A string representing the market update.
     */
    [[nodiscard]] std::string toStr() const {
        return std::format("<OMEMarketUpdate> [type: {}, ticker: {}, oid: {}, side: {}, qty: {}, price: {}, priority: {}]",
                           typeToStr(type),
                           tickerIdToStr(tickerId), orderIdToStr(orderId), sideToStr(side), qtyToStr(qty), priceToStr(price),
                           priorityToStr(priority));
    }
};

/**
 * @brief Market update format sent by the Market Data Publisher
 * for public exchange clients to consume.
 * @details Data is disseminated over UDP, so the nSeq member
 * keeps track of data sequence order to allow clients to
 * reconstruct the correct market data order on their end.
 */
struct MDPMarketUpdate {
    std::size_t nSeq{0};        // Sequence number for ordering
    OMEMarketUpdate omeUpdate;  // The actual market update

    /**
     * @brief Converts the MDP market update to a string representation.
     * @return A string representing the MDP market update.
     */
    [[nodiscard]] std::string toStr() const {
        return std::format("<MDPMarketUpdate> [nSeq: {}, {}]", nSeq, omeUpdate.toStr());
    }
};

#pragma pack(pop) // Restore default alignment

// Queue for updates from OrderMatchingEngine to MarketDataPublisher
using MarketUpdateQueue = utils::LFQueue<OMEMarketUpdate>;

// Queue for updates from MarketDataPublisher to public exchange clients
using MDPMarketUpdateQueue = utils::LFQueue<MDPMarketUpdate>;

} // namespace Exchange

#endif // LOW_LATENCY_TRADING_APP_MARKET_DATA_MSGS_H