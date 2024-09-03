//
// Created by ABDERRAHIM ZEBIRI on 2024-08-06.
//

#include "./utils/thread_util.h"
#include "utils/lock_free_queue.h"
#include "utils/logger.h"
#include "utils/tcp_server.h"
#include "utils/tcp_socket.h"
//#include <benchmark/benchmark.h>

//static void BM_LOGGING(benchmark::State& state) {
//    for (auto _ : state) {
//        LOG_INFO("This is an info message" + std::to_string(56));
//    }
//}
//BENCHMARK(BM_LOGGING);
//
//static void BM_LOGGINGF(benchmark::State& state) {
//    for (auto _ : state) {
//        LOG_INFO("This is an {} info message with a formatted value: {}", 56, "formatted");
//
//    }
//}
//BENCHMARK(BM_LOGGINGF);
//
//BENCHMARK_MAIN();

int main(int, char **) {
    using namespace utils;

    auto tcpServerRecvCallback = [](TCPSocket* socket, Nanos rx_time) noexcept {
        if (!socket) {
            LOG_ERROR("Received null socket in callback");
            return;
        }
        int socketFd = socket->getSocketFd();
        size_t i = socket->getNextRcvValidIndex();
        LOG_INFO("TCPServer::defaultRecvCallback() socket:{} len:{} msg:{}", socketFd, i, rx_time);

        const std::string reply = "TCPServer received msg:" +
                                  std::string(reinterpret_cast<const char*>(socket->getInboundData().data()), i);
        socket->restNextRcvValidIndex();
        socket->send(reinterpret_cast<const std::byte*>(reply.data()), reply.size());
    };

    auto tcpServerRecvFinishedCallback = []() noexcept { LOG_INFO("TCPServer::defaultRecvFinishedCallback()");
    };

    auto tcpClientRecvCallback = [](TCPSocket *socket, Nanos rx_time) noexcept {
        const size_t i = socket->getNextRcvValidIndex();
        const std::string recv_msg(reinterpret_cast<const char *>(socket->getInboundData().data()), i);
        socket->restNextRcvValidIndex();

        const int fd = socket->getSocketFd();
        LOG_INFO("TCPClient::defaultRecvCallback() socket:{} len:{} msg:{}", fd, i, recv_msg);
    };

    const std::string iface = "lo0";
    const std::string ip = "127.0.0.1";
    const int port = 12345;

    LOG_INFO("Starting TCP server on iface:{} port:{}", iface, port);

    TCPServer server;
    server.setRecvCallback(tcpServerRecvCallback);
    server.setRecvFinishedCallback(tcpServerRecvFinishedCallback);
    server.listen(iface, port);

    std::vector<std::unique_ptr<TCPSocket>> clients(2);

    for (size_t i = 0; i < clients.size(); ++i) {
        clients[i] = std::make_unique<TCPSocket>();
        clients[i]->setRecvCallback(tcpClientRecvCallback);

        LOG_INFO("Connecting TCPClient[{}] on iface:{} port:{}", i, iface, port);
        clients[i]->connect(ip, iface, port, false);
        server.poll();
    }

    using namespace std::literals::chrono_literals;

    for (auto itr = 0; itr < 1; ++itr) {
        for (size_t i = 0; i < clients.size(); ++i) {
            const std::string client_msg = std::format("Client[{}] message iteration:{}", i, itr);
            LOG_INFO("Sending message:{}", client_msg);

            clients[i]->send(client_msg.data(), client_msg.length());
            clients[i]->sendAndRecv();

            std::this_thread::sleep_for(500ms);
            server.poll();
            server.sendAndReceive();
        }
    }

    return 0;
}
