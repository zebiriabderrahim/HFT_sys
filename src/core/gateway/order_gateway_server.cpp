//
// Created by ABDERRAHIM ZEBIRI on 2024-09-19.
//

#include "order_gateway_server.h"

#include "lib/logger.h"
#include "lib/thread_util.h"

namespace Exchange {

OrderGatewayServer::OrderGatewayServer(ClientRequestQueue& txRequests,
                                       ClientResponseQueue& rxResponses,
                                       std::string_view iface, int port) noexcept
    : iface_(iface), port_(port), rxResponses_(rxResponses), fifo_(txRequests) {
    mapClientToTxNSeq_.fill(1);
    mapClientToRxNSeq_.fill(1);
    mapClientToSocket_.fill(nullptr);

    server_.setRecvCallback([this](auto socket, auto tRx) { rxCallback(socket, tRx); });
    server_.setRecvFinishedCallback([this]() { rxDoneCallback(); });
}

OrderGatewayServer::~OrderGatewayServer() {
    stop();
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
}

void OrderGatewayServer::start() noexcept {
    isRunning_ = true;
    server_.listen(iface_, port_);
    serverThread_ = utils::createAndStartThread(-1, "OrderGatewayServer", [this]() { run(); });
    ASSERT_CONDITION(serverThread_ != nullptr, "<OGS> Failed to start thread for order gateway");
}

void OrderGatewayServer::stop() noexcept {
    isRunning_ = false;
    if (serverThread_->joinable()) {
        serverThread_->join();
    }
}

void OrderGatewayServer::run() noexcept {
    LOG_INFO("OrderGatewayServer running order gateway...");
    while (isRunning_) {
        server_.poll();
        server_.sendAndReceive();

        for (auto res = rxResponses_.getNextToRead(); rxResponses_.size() && res; res = rxResponses_.getNextToRead()) {
            auto& nSeqTxNext = mapClientToTxNSeq_[res->clientId];

            LOG_INFO("Processing client id {} with seq number {} and response: {}", res->clientId, nSeqTxNext, res->toStr());
            ASSERT_CONDITION(mapClientToSocket_[res->clientId] != nullptr, "<OGS> missing socket for client: {}", res->clientId);
            mapClientToSocket_[res->clientId]->send(&nSeqTxNext, sizeof(nSeqTxNext));
            mapClientToSocket_[res->clientId]->send(res, sizeof(OMEClientResponse));

            rxResponses_.updateReadIndex();

            ++nSeqTxNext;
        }
    }
}

void OrderGatewayServer::rxCallback(utils::TCPSocket* socket, utils::Nanos tRx) noexcept {
    LOG_INFO("Received {} bytes from socket: {}", socket->getNextRcvValidIndex(), socket->getSocketFd());

    // Available rx data should be at least one client request in size
    if (socket->getNextRcvValidIndex() >= sizeof(OGSClientRequest)) {
        size_t i = 0;
        for (; i + sizeof(OGSClientRequest) <= socket->getNextRcvValidIndex(); i += sizeof(OGSClientRequest)) {
            auto req = reinterpret_cast<const OGSClientRequest*>(socket->getInboundData().data() + i);
            LOG_INFO("Received OGSClientRequest: {}", req->toStr());

            // Client's first order req; start tracking with a new socket mapping
            if (mapClientToSocket_[req->omeRequest.clientId] == nullptr) [[unlikely]] {
                mapClientToSocket_[req->omeRequest.clientId] = socket;
            }

            // Current req socket does not match the mapped one
            if (mapClientToSocket_[req->omeRequest.clientId]!= socket) {
                // Todo: This should send a response back to client
                LOG_ERROR("Received request from client: {} on socket: {}! Expected: {}",
                          req->omeRequest.clientId, socket->getSocketFd(),
                          mapClientToSocket_[req->omeRequest.clientId]->getSocketFd());
                continue;
            }

            // Sanity check for sequence number
            auto& nSeqRxNext = mapClientToRxNSeq_[req->omeRequest.clientId];
            if (req->nSeq!= nSeqRxNext) {
                // Todo: This should send a rejection back to client

                LOG_ERROR("Received seq number error! client: {} n_seq expected: {} but received: {}",
                          req->omeRequest.clientId, nSeqRxNext, req->nSeq);
                continue;
            }

            // Increment client seq number and forward order to the exchange FIFO sequencer
            ++nSeqRxNext;
            fifo_.pushClientRequest(req->omeRequest, tRx);
        }

        std::memcpy((void*)socket->getInboundData().data(), socket->getInboundData().data() + i,
                    socket->getNextRcvValidIndex() - i);
        socket->setNextRcvValidIndex(socket->getNextRcvValidIndex() - i);
    }
}

void OrderGatewayServer::rxDoneCallback() noexcept {
    fifo_.sequenceAndPublish();
}

} // namespace Exchange