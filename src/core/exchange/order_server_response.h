//
// Created by ABDERRAHIM ZEBIRI on 2024-09-16.
//

#ifndef LOW_LATENCY_TRADING_APP_ORDER_SERVER_RESPONSE_H
#define LOW_LATENCY_TRADING_APP_ORDER_SERVER_RESPONSE_H


#include <string>
#include <string_view>
#include <format>

#include "types.h"
#include "lib/lock_free_queue.h"

namespace Exchange {

// Force 1-byte alignment for efficient network transmission and queue traversal
#pragma pack(push, 1)

/**
 * @brief Message from the OME to a client.
 *
 * This response is passed to the Order Server which encodes
 * and forwards it to the market participant.
 */
struct OMEClientResponse {
    enum class Type : std::uint8_t {
        INVALID = 0,
        ACCEPTED = 1,
        CANCELLED = 2,
        FILLED = 3,
        CANCEL_REJECTED = 4,
    };

    Type type{Type::INVALID};                   ///< Message type
    ClientID clientId{ClientID_INVALID};        ///< Client the message is for
    TickerID tickerId{TickerID_INVALID};        ///< Ticker of product
    OrderID clientOrderId{OrderID_INVALID};     ///< Order ID from original request
    OrderID marketOrderId{OrderID_INVALID};     ///< Market-wide published order ID
    Side side{Side::INVALID};                   ///< Buy or sell
    Price price{Price_INVALID};                 ///< Price of order
    Qty qtyExec{Qty_INVALID};                   ///< Executed quantity
    Qty qtyRemain{Qty_INVALID};                 ///< Remaining quantity

    bool operator==(const OMEClientResponse&) const = default;

    [[nodiscard]] static constexpr std::string_view typeToStr(Type type) noexcept {
        switch (type) {
            using enum Exchange::OMEClientResponse::Type;
        case ACCEPTED: return "ACCEPTED";
        case CANCELLED: return "CANCELLED";
        case FILLED: return "FILLED";
        case CANCEL_REJECTED: return "CANCEL_REJECTED";
        case INVALID: return "INVALID";
        default: return "UNKNOWN";
        }
    }

    [[nodiscard]] std::string toStr() const {
        return std::format("<OMEClientResponse> [type: {}, client: {}, ticker: {}, "
                           "oid_client: {}, oid_market: {}, side: {}, qty_exec: {}, "
                           "qty_remain: {}, price: {}]",
                           typeToStr(type), clientIdToStr(clientId), tickerIdToStr(tickerId),
                           orderIdToStr(clientOrderId), orderIdToStr(marketOrderId),
                           sideToStr(side), qtyToStr(qtyExec), qtyToStr(qtyRemain),
                           priceToStr(price));
    }
};

/**
 * @brief An order response sent from the Order Gateway Server to a market participant.
 */
struct OGSClientResponse {
    std::size_t nSeq{0};
    OMEClientResponse omeResponse;

    [[nodiscard]] std::string toString() const {
        return std::format("<OGSClientResponse> [n: {}, {}]", nSeq, omeResponse.toStr());
    }
};

#pragma pack(pop)

/// Queue for responses from OrderMatchingEngine to OrderServer
using ClientResponseQueue = utils::LFQueue<OMEClientResponse>;

} // namespace Exchange

#endif // LOW_LATENCY_TRADING_APP_ORDER_SERVER_RESPONSE_H
