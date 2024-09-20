//
// Created by ABDERRAHIM ZEBIRI on 2024-08-06.
//

#include "lib/logger.h"
#include "lib/tcp_server.h"
#include "lib/tcp_socket.h"

#include "core/exchange/market_data.h"
#include "core/exchange/order_server_request.h"
#include "core/exchange/order_server_response.h"
#include "core/exchange/types.h"
#include "core/matching_engine/matching_engine.h"
#include "core/gateway/order_gateway_server.h"

#include <csignal>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
//#include <benchmark/benchmark.h>
//
//static void BM_LOGGING(benchmark::State& state) {
//    for (auto _ : state) {
//        std::string msg = "This is an info message";
//        msg = std::format("babab {}", msg);
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
//
void shutdown_handler(int) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);
    std::this_thread::sleep_for(5s);
    exit(EXIT_SUCCESS);
}
int main(int, char **) {
    using namespace Exchange;
    using namespace utils;
    std::signal(SIGINT, shutdown_handler);

    ClientRequestQueue client_requests{ Types::MAX_CLIENT_UPDATES };
    ClientResponseQueue client_responses{ Types::MAX_CLIENT_UPDATES };
    MarketUpdateQueue market_updates{ Types::MAX_MARKET_UPDATES };

    // start the matching engine
    LOG_INFO("Starting matching engine...");



   auto ome = std::make_unique<MatchingEngine::MatchingEngine>(client_requests,
                                                               client_responses,
                                                               market_updates);
    ome->startMatchingEngine();
    // main exchange superloop
    const int t_sleep{ 100 * 1000 };
    while (true) {
        LOG_INFO("Sleeping for some ms...");
        usleep(t_sleep);    // sleep which can be terminated by a SIGINT/etc.
    }

    return 0;
}
