//
// Created by ABDERRAHIM ZEBIRI on 2024-09-09.
//
#ifndef LOW_LATENCY_TRADING_APP_ORDER_SERVER_REQUEST_H
#define LOW_LATENCY_TRADING_APP_ORDER_SERVER_REQUEST_H

#include <string>
#include <format>
#include <cstdint>

#include "types.h"
#include "lib/lock_free_queue.h"

namespace Exchange {

/**
 * @brief Namespace containing message structures for order processing
 * @details This namespace defines structures used for communication between
 * the Order Server, Order Matching Engine (OME), and Order Gateway Server (OGS)
 * in a high-frequency trading environment.
 */

/**
 * @brief Force 1-byte alignment for all structures in this scope
 * @details This is crucial for efficient network transmission and queue traversal
 * in high-frequency trading systems. It prioritizes compact data representation
 * over potential compiler speed optimizations.
 */
#pragma pack(push, 1)

/**
 * @brief Represents an order request passed from the Order Server to the Order Matching Engine
 * @details This structure encapsulates all necessary information for processing a client's order
 */
struct OMEClientRequest {
    /**
     * @brief Enumeration of possible request types
     */
    enum class Type : std::uint8_t {
        INVALID = 0, ///< Invalid or uninitialized request
        NEW = 1,     ///< New order request
        CANCEL = 2   ///< Cancel existing order request
    };

    Type type{Type::INVALID};               ///< Type of the request
    ClientID clientId{ClientID_INVALID};   ///< ID of the client making the request
    TickerID tickerId{TickerID_INVALID};   ///< ID of the product being traded
    OrderID orderId{OrderID_INVALID};      ///< ID of the order (new or existing)
    Side side{Side::INVALID};               ///< Buy or sell side of the order
    Price price{Price_INVALID};             ///< Price of the order
    Qty qty{Qty_INVALID};                   ///< Quantity of the order

    /**
     * @brief Default comparison operator
     */
    bool operator==(const OMEClientRequest&) const = default;

    /**
     * @brief Converts the request type to a string representation
     * @param type The type to convert
     * @return A string view representing the type
     */
    [[nodiscard]] static constexpr auto typeToStr(Type type) noexcept-> std::string_view {
        switch (type) {
            using enum Type;
        case NEW: return "NEW";
        case CANCEL: return "CANCEL";
        case INVALID: return "INVALID";
        default: return "UNKNOWN";
        }
    }

    /**
     * @brief Converts the entire request to a string representation
     * @return A string representing all fields of the request
     */
    [[nodiscard]] auto toStr() const -> std::string {
        return std::format("<OMEClientRequest> [type: {}, clientId: {}, tickerId: {}, orderId: {}, side: {}, price: {}, qty: {}]",
                           typeToStr(type), clientIdToStr(clientId), tickerIdToStr(tickerId),
                           orderIdToStr(orderId), sideToStr(side), priceToStr(price), qtyToStr(qty));
    }
};

/**
 * @brief Represents an order request sent from a public exchange client to the Order Gateway Server
 * @details This structure wraps an OMEClientRequest with an additional sequence number
 */
struct OGSClientRequest {
    std::size_t nSeq{0};        ///< Sequence number for ordering requests
    OMEClientRequest omeRequest; ///< The actual order request

    /**
     * @brief Converts the entire request to a string representation
     * @return A string representing all fields of the request
     */
    [[nodiscard]] auto toStr() const -> std::string {
        return std::format("<OGSClientRequest> [nSeq: {}, omeRequest: {}]", nSeq, omeRequest.toStr());
    }
};

#pragma pack(pop) // Restore default alignment

/**
 * @brief A lock-free queue for client requests
 * @details Used for passing requests from the Order Matching Engine to the Order Server
 */
using ClientRequestQueue = utils::LFQueue<OMEClientRequest>;

} // namespace Exchange

#endif // LOW_LATENCY_TRADING_APP_ORDER_SERVER_REQUEST_H