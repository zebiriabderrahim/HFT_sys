//
// Created by ABDERRAHIM ZEBIRI on 2024-09-09.
//

#ifndef LOW_LATENCY_TRADING_APP_TYPES_H
#define LOW_LATENCY_TRADING_APP_TYPES_H

#include <cstdint>
#include <limits>
#include <cstddef>
#include <string>
#include <string_view>

// Uncomment this line for release build
// #undef IS_TEST_SUITE

namespace Exchange {

/**
 * @brief Order Matching Engine types and constants
 */
namespace Types {
// Exchange system limits - can be modified and tuned
#ifdef IS_TEST_SUITE
/// @brief Size of Order Matching Engine for test suite
inline constexpr std::size_t OME_SIZE = 16;
#else
/// @brief Size of Order Matching Engine for production
inline constexpr std::size_t OME_SIZE = 256;
#endif

/// @brief Maximum number of trading instruments supported
inline constexpr std::size_t MAX_TICKERS = 8;
/// @brief Maximum number of client updates (matching requests and responses) that can be queued
inline constexpr std::size_t MAX_CLIENT_UPDATES = OME_SIZE * 1024;
/// @brief Maximum number of market updates that can be queued for publishing
inline constexpr std::size_t MAX_MARKET_UPDATES = OME_SIZE * 1024;
/// @brief Maximum number of market participants
inline constexpr std::size_t MAX_N_CLIENTS = OME_SIZE;
/// @brief Maximum number of orders for a single trading instrument
inline constexpr std::size_t MAX_ORDER_IDS = 1024 * 1024;
/// @brief Maximum depth of price levels in the order book
inline constexpr std::size_t MAX_PRICE_LEVELS = OME_SIZE;
/// @brief Maximum number of pending requests on order gateway socket
inline constexpr std::size_t MAX_PENDING_ORDER_REQUESTS = 1024;
}

/**
 * @brief Const template for invalid ID values
 * @tparam T The numeric type for which to define an invalid value
 */
template<typename T>
inline constexpr T INVALID_ID = std::numeric_limits<T>::max();

/**
 * @brief Convert a numeric literal to string
 * @tparam T Type of numeric to convert
 * @param id The numeric value to convert
 * @return String view representation or "INVALID" if it's the invalid value
 */
template<typename T>
[[nodiscard]] inline auto numericToStr(T id) noexcept -> std::string {
    if (id == INVALID_ID<T>) [[unlikely]] {
        return "INVALID";
    }
    return std::to_string(id);
}

/// @brief Type for unique order identification
using OrderID = std::uint64_t;
/// @brief Invalid value for OrderID
inline constexpr auto OrderID_INVALID = INVALID_ID<OrderID>;

/// @brief Convert OrderID to string representation
[[nodiscard]] inline auto orderIdToStr(OrderID id) noexcept-> std::string {
    return numericToStr(id);
}

/// @brief Type for unique product ticker identification
using TickerID = std::uint32_t;
/// @brief Invalid value for TickerID
inline constexpr auto TickerID_INVALID = INVALID_ID<TickerID>;

/// @brief Convert TickerID to string representation
[[nodiscard]] inline auto tickerIdToStr(TickerID id) noexcept -> std::string {
    return numericToStr(id);
}

/// @brief Type for unique client identification in the exchange
using ClientID = std::uint32_t;
/// @brief Invalid value for ClientID
inline constexpr auto ClientID_INVALID = INVALID_ID<ClientID>;

/// @brief Convert ClientID to string representation
[[nodiscard]] inline auto clientIdToStr(ClientID id) noexcept -> std::string {
    return numericToStr(id);
}

/// @brief Type for price representation
using Price = std::int64_t;
/// @brief Invalid value for Price
inline constexpr auto Price_INVALID = INVALID_ID<Price>;
/// @brief Convert Price to string representation
[[nodiscard]] inline auto priceToStr(Price price) noexcept -> std::string {
    return numericToStr(price);
}

/// @brief Type for quantity representation
using Qty = std::uint32_t;
/// @brief Invalid value for Qty
inline constexpr auto Qty_INVALID = INVALID_ID<Qty>;
/// @brief Convert Qty to string representation
[[nodiscard]] inline auto qtyToStr(Qty qty) noexcept -> std::string {
    return numericToStr(qty);
}

/// @brief Type for order priority in the FIFO matching queue
using Priority = std::uint64_t;
/// @brief Invalid value for Priority
inline constexpr auto Priority_INVALID = INVALID_ID<Priority>;

/// @brief Convert Priority to string representation
[[nodiscard]] inline auto priorityToStr(Priority priority) noexcept -> std::string {
    return numericToStr(priority);
}

/**
 * @brief Enum representing the side of a trade
 * @details Uses int8_t for optimal bit-packing in network transmission
 */
enum class Side : std::int8_t {
    INVALID = 0,
    BUY = 1,
    SELL = -1
};

/**
 * @brief Convert Side enum to string representation
 * @param side The Side enum value to convert
 * @return String view representation of the Side
 */
[[nodiscard]] constexpr std::string_view sideToStr(Side side) noexcept {
    switch (side) {
        using enum Side;
    case BUY: return "BUY";
    case SELL: return "SELL";
    case INVALID: return "INVALID";
    default: return "UNKNOWN";
    }
}

} // namespace Exchange

#endif // LOW_LATENCY_TRADING_APP_TYPES_H
