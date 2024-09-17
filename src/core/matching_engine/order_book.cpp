#include "order_book.h"
#include "matching_engine.h"
#include <algorithm>
#include <format>

namespace MatchingEngine {

OrderBook::OrderBook(Exchange::TickerID assignedTicker, MatchingEngine &ome) noexcept : assignedTicker_(assignedTicker), ome_(ome) {}

OrderBook::~OrderBook() {
    // Log the final state of the order book
    LOG_INFO("{}\n", toString(false, true));
    bidsByPrice_ = asksByPrice_ = nullptr;
    for (auto &oids : mapClientIdToOrder_) {
        oids.fill(nullptr);
    }
}

void OrderBook::addOrder(Exchange::ClientID clientId, Exchange::OrderID clientOid, Exchange::TickerID tickerId, Exchange::Side side,
                         Exchange::Price price, Exchange::Qty qty) noexcept {
    const auto newMarketOid = getNewMarketOrderId();
    clientResponse_ = {Exchange::OMEClientResponse::Type::ACCEPTED, clientId, tickerId, clientOid, newMarketOid, side, price, 0, qty};
    ome_.dispatchClientResponse(clientResponse_);

    const auto qtyRemains = findMatch(clientId, clientOid, tickerId, side, price, qty, newMarketOid);
    if (qtyRemains) [[likely]] {
        const auto priority = getNextPriority(price);
        auto order = orderPool_.allocate(tickerId, clientId, clientOid, newMarketOid, side, price, qtyRemains, priority, nullptr, nullptr);
        addOrderToBook(order);

        marketUpdate_ = {Exchange::OMEMarketUpdate::Type::ADD, newMarketOid, tickerId, side, price, qtyRemains, priority};
        ome_.publishMarketUpdate(marketUpdate_);
    }
}

void OrderBook::cancelOrder(Exchange::ClientID clientId, Exchange::OrderID orderId, Exchange::TickerID tickerId) noexcept {
    bool canBeCanceled = (clientId < mapClientIdToOrder_.size());
    Exchange::Order *exchangeOrder = nullptr;

    if (canBeCanceled) [[likely]] {
        auto &mapClientOrder = mapClientIdToOrder_.at(clientId);
        exchangeOrder = mapClientOrder.at(orderId);
        canBeCanceled = (exchangeOrder != nullptr);
    }

    if (!canBeCanceled) [[unlikely]] {
        clientResponse_ = {Exchange::OMEClientResponse::Type::CANCEL_REJECTED,
                           clientId, tickerId,
                           orderId,Exchange::OrderID_INVALID, Exchange::Side::INVALID,
                           Exchange::Price_INVALID, Exchange::Qty_INVALID,
                           Exchange::Qty_INVALID};
    } else {
        clientResponse_ = {Exchange::OMEClientResponse::Type::CANCELLED,
                           clientId, tickerId,
                           orderId, exchangeOrder->marketOrderId_,
                           exchangeOrder->side_,exchangeOrder->price_,
                           Exchange::Qty_INVALID,exchangeOrder->qty_};

        marketUpdate_ = {Exchange::OMEMarketUpdate::Type::CANCEL,
                         exchangeOrder->marketOrderId_, tickerId,
                         exchangeOrder->side_, exchangeOrder->price_,
                         0, exchangeOrder->priority_};

        removeOrderFromBook(exchangeOrder);
        ome_.publishMarketUpdate(marketUpdate_);
    }
    ome_.dispatchClientResponse(clientResponse_);
}

Exchange::Qty OrderBook::findMatch(Exchange::ClientID clientId, Exchange::OrderID clientOid,
                                   Exchange::TickerID tickerId, Exchange::Side side,
                                   Exchange::Price price, Exchange::Qty qty,
                                   Exchange::OrderID newMarketOid) noexcept {
    auto qtyRemains = qty;
    if (side == Exchange::Side::BUY) {
        while (qtyRemains && asksByPrice_) {
            const auto asksList = asksByPrice_->order0_;
            if (price < asksList->price_) [[likely]] {
                break;
            }
            matchOrder(tickerId, clientId, side, clientOid, newMarketOid, asksList, &qtyRemains);
        }
    } else if (side == Exchange::Side::SELL) {
        while (qtyRemains && bidsByPrice_) {
            const auto bidsList = bidsByPrice_->order0_;
            if (price > bidsList->price_) [[likely]] {
                break;
            }
            matchOrder(tickerId, clientId, side, clientOid, newMarketOid, bidsList, &qtyRemains);
        }
    }
    return qtyRemains;
}

void OrderBook::matchOrder(Exchange::TickerID tickerId, Exchange::ClientID clientId,
                           Exchange::Side side, Exchange::OrderID clientOrderId,
                           Exchange::OrderID newMarketOid, Exchange::Order *orderMatched,
                           Exchange::Qty *qtyRemains) noexcept {
    const auto fillQty = std::min(*qtyRemains, orderMatched->qty_);
    qtyRemains -= fillQty;
    orderMatched->qty_ -= fillQty;

    clientResponse_ = { Exchange::OMEClientResponse::Type::FILLED,
                       clientId, tickerId,
                       clientOrderId, newMarketOid,
                       side, orderMatched->price_,
                       fillQty, *qtyRemains };
    ome_.dispatchClientResponse(clientResponse_);

    clientResponse_ = { Exchange::OMEClientResponse::Type::FILLED,
                       orderMatched->clientId_,tickerId,
                       orderMatched->clientOrderId_,orderMatched->marketOrderId_,
                       orderMatched->side_,orderMatched->price_,
                       fillQty,orderMatched->qty_ };
    ome_.dispatchClientResponse(clientResponse_);

    marketUpdate_ = { Exchange::OMEMarketUpdate::Type::TRADE,
                     Exchange::OrderID_INVALID, tickerId,
                     side, orderMatched->price_,
                     fillQty, Exchange::Priority_INVALID};
    ome_.publishMarketUpdate(marketUpdate_);

    if (!orderMatched->qty_) {
        marketUpdate_ = { Exchange::OMEMarketUpdate::Type::CANCEL,
                         orderMatched->marketOrderId_, tickerId,
                         orderMatched->side_,orderMatched->price_,
                         fillQty,Exchange::Priority_INVALID};
        ome_.publishMarketUpdate(marketUpdate_);
        removeOrderFromBook(orderMatched);
    } else {
        marketUpdate_ = {Exchange::OMEMarketUpdate::Type::MODIFY,
                         orderMatched->marketOrderId_,tickerId,
                         orderMatched->side_,orderMatched->price_,
                         orderMatched->qty_,orderMatched->priority_};
        ome_.publishMarketUpdate(marketUpdate_);
    }
}

// ... (implement other methods like addOrderToBook, removeOrderFromBook, etc.)

std::string OrderBook::toString(bool isDetailed, bool hasValidityCheck) const {
    std::string result;
    result.reserve(4096);  // Pre-allocate space to avoid frequent reallocations

    auto printer = [&](std::string_view side, const Exchange::OrdersAtPrice* levels,
                       Exchange::Side sideEnum, Exchange::Price& lastPrice) {
        Exchange::Qty totalQty{0};
        size_t orderCount = 0;

        // Count orders and total quantity
        for (auto order = levels->order0_; ; order = order->next_) {
            totalQty += order->qty_;
            ++orderCount;
            if (order->next_ == levels->order0_) break;
        }

        result += std::format(" {{ p:{:3} [-]:{:3} [+]:{:3} }} {:5} @ {:3} ({:4})\n",
                              Exchange::priceToStr(levels->price_),
                              Exchange::priceToStr(levels->prev_->price_),
                              Exchange::priceToStr(levels->next_->price_),
                              Exchange::qtyToStr(totalQty),
                              Exchange::priceToStr(levels->price_),
                              orderCount);

        if (isDetailed) {
            for (auto order = levels->order0_; ; order = order->next_) {
                result += std::format("\t\t\t{{ oid:{}, q:{}, p:{}, n:{} }}\n",
                                      Exchange::orderIdToStr(order->marketOrderId_),
                                      Exchange::qtyToStr(order->qty_),
                                      Exchange::orderIdToStr(order->prev_ ? order->prev_->marketOrderId_ : Exchange::OrderID_INVALID),
                                      Exchange::orderIdToStr(order->next_ ? order->next_->marketOrderId_ : Exchange::OrderID_INVALID));
                if (order->next_ == levels->order0_) break;
            }
        }

        if (hasValidityCheck) {
            bool isSorted = (sideEnum == Exchange::Side::SELL && lastPrice < levels->price_) ||
                            (sideEnum == Exchange::Side::BUY && lastPrice > levels->price_);
            if (!isSorted) {
                throw std::runtime_error(std::format("Bid/ask price levels not sorted correctly: {} levels:{}",
                                                     Exchange::priceToStr(lastPrice), levels->toString()));
            }
            lastPrice = levels->price_;
        }
    };

    result += std::format("\n----- ORDER BOOK FOR TICKER: {} -----\n", Exchange::tickerIdToStr(assignedTicker_));

    // Print asks
    {
        auto asks = asksByPrice_;
        auto lastAskPrice = std::numeric_limits<Exchange::Price>::min();
        if (asks == nullptr) {
            result += "\n                  [NO ASKS ON BOOK]\n";
        } else {
            for (size_t count = 0; asks; ++count) {
                result += std::format("ASKS[{}] => ", count);
                printer("ASK", asks, Exchange::Side::SELL, lastAskPrice);
                asks = (asks->next_ == asksByPrice_) ? nullptr : asks->next_;
            }
        }
    }

    result += "\n                          X\n\n";

    // Print bids
    {
        auto bids = bidsByPrice_;
        auto lastBidPrice = std::numeric_limits<Exchange::Price>::max();
        if (bids == nullptr) {
            result += "\n                  [NO BIDS ON BOOK]\n";
        } else {
            for (size_t count = 0; bids; ++count) {
                result += std::format("BIDS[{}] => ", count);
                printer("BID", bids, Exchange::Side::BUY, lastBidPrice);
                bids = (bids->next_ == bidsByPrice_) ? nullptr : bids->next_;
            }
        }
    }

    return result;
}
void OrderBook::addPriceLevel(Exchange::OrdersAtPrice *newOrdersAtPrice) noexcept {}
void OrderBook::removePriceLevel(Exchange::Side side, Exchange::Price price) noexcept {}
void OrderBook::addOrderToBook( Exchange::Order *order) noexcept {}
void OrderBook::removeOrderFromBook(Exchange::Order* order) noexcept {
    auto ordersAtPrice = getLevelForPrice(order->price_);

    if (order->prev_ == order) {
        // It's the only order at this price level; remove the entire level
        removePriceLevel(order->side_, order->price_);
    } else {
        // Remove the order from the doubly-linked list
        order->prev_->next_ = order->next_;
        order->next_->prev_= order->prev_;

        if (ordersAtPrice->order0_ == order) {
            ordersAtPrice->order0_ = order->next_;
        }

        // Clear the order's links
        order->prev_ = order->next_ = nullptr;
    }

    // Remove from client order map
    mapClientIdToOrder_[order->clientId_][order->clientOrderId_] = nullptr;

    // Deallocate the order
    orderPool_.deallocate(order);
}

} // namespace MatchingEngine