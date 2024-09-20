// Created by ABDERRAHIM ZEBIRI on 2024-08-22.

#ifndef LOW_LATENCY_TRADING_APP_SOCKET_UTILS_H
#define LOW_LATENCY_TRADING_APP_SOCKET_UTILS_H

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <ifaddrs.h>
#include <iostream>
#include <netdb.h>
#include <netinet/tcp.h>
#include <source_location>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

#include "assertion.h"
#include "logger.h"

namespace utils {
/**
 * @brief Maximum number of connections that can be queued for a TCP server.
 */
constexpr int MaxTCPServerBacklog = 1024;

/**
 * @brief Configuration for socket settings.
 */
struct SocketConfig {
    std::string ipAddress;                        ///< IP address of the socket.
    std::string interfaceName;                    ///< Network interface name.
    int portNumber;                               ///< Port number.
    bool useUdp = false;                          ///< Flag indicating if UDP protocol is used.
    bool isListeningMode = false;                 ///< Flag indicating if the socket is in listening mode.
    bool enableTimestamp = false;                 ///< Flag for enabling SO_TIMESTAMP option.

    /**
     * @brief Convert the socket configuration to a string representation.
     * @return A string detailing the socket configuration.
     */
    [[nodiscard]] auto toString() const {
        return std::format("ip:{} interface:{} port:{} udp:{} listening:{} timestamp:{}",
                           ipAddress, interfaceName, portNumber, useUdp, isListeningMode, enableTimestamp);
    }
};

/**
 * @brief Get the IP address of a network interface.
 * @param interfaceName The name of the network interface.
 * @return The IP address as a string.
 */
[[nodiscard]] inline auto getIpAddressForInterface(std::string_view interfaceName) -> std::string {
    char buf[NI_MAXHOST] = {'\0'};

    if (ifaddrs *ifaddr = nullptr; getifaddrs(&ifaddr) != -1) {
        for (ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && interfaceName == ifa->ifa_name) {
                getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST);
                break;
            }
        }
        freeifaddrs(ifaddr);
    }

    return buf;
}

/**
 * @brief Set a socket to non-blocking mode.
 * @param socketFd The file descriptor of the socket.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto setSocketNonBlocking(int socketFd) -> bool {
    const auto flags = fcntl(socketFd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) != -1);
}

/**
 * @brief Disable the Nagle algorithm for a socket.
 * @param socketFd The file descriptor of the socket.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto disableNagleAlgorithm(int socketFd) -> bool {
    int flag = 1;
    return (setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) != -1);
}

/**
 * @brief Enable SO_TIMESTAMP on a socket.
 * @param socketFd The file descriptor of the socket.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto enableSocketTimestamp(int socketFd) -> bool {
    int flag = 1;
    return (setsockopt(socketFd, SOL_SOCKET, SO_TIMESTAMP, &flag, sizeof(int)) != -1);
}

/**
 * @brief Join a multicast group on the specified interface.
 * @param fd The file descriptor of the socket.
 * @param multicastIp The IP address of the multicast group to join.
 * @param interfaceIp The IP address of the local interface to use (optional, defaults to INADDR_ANY).
 * @return True if successfully joined the multicast group.
 * @throws std::system_error if an error occurs while joining the multicast group.
 */
inline auto joinMulticastGroup(int fd, std::string_view multicastIp, std::string_view interfaceIp = "0.0.0.0") -> bool {
    struct ip_mreq mreq {};
    mreq.imr_multiaddr.s_addr = inet_addr(multicastIp.data());
    mreq.imr_interface.s_addr = inet_addr(interfaceIp.data());
    return (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1);
}

/**
 * @brief Create a socket based on the configuration.
 * @param socketConfig The socket configuration.
 * @return The file descriptor of the created socket.
 */
[[nodiscard]] inline auto createSocket(const SocketConfig &socketConfig) -> int {
    const auto ip = socketConfig.ipAddress.empty() ? getIpAddressForInterface(socketConfig.interfaceName) : socketConfig.ipAddress;

    LOG_INFO("Creating socket with configuration:{}", socketConfig.toString());

    const int inputFlags = (socketConfig.isListeningMode ? AI_PASSIVE : 0) | (AI_NUMERICHOST | AI_NUMERICSERV);
    const addrinfo hints{
        inputFlags,
        AF_INET,
        socketConfig.useUdp ? SOCK_DGRAM : SOCK_STREAM,
        socketConfig.useUdp ? IPPROTO_UDP : IPPROTO_TCP,
        0,
        nullptr,
        nullptr,
        nullptr};

    addrinfo *result = nullptr;
    const auto rc = getaddrinfo(ip.c_str(), std::to_string(socketConfig.portNumber).c_str(), &hints, &result);
    ASSERT_CONDITION(!rc, "getaddrinfo() failed. error: {}  errno: {}", std::string(gai_strerror(rc)), std::string(strerror(errno)));

    int socketFd = -1;
    int one = 1;
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
        socketFd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        ASSERT_CONDITION(socketFd != -1, "socket() failed. errno:{} ", std::string(strerror(errno)));

        ASSERT_CONDITION(setSocketNonBlocking(socketFd), "setNonBlocking() failed. errno: {}", std::string(strerror(errno)));

        if (!socketConfig.useUdp) {
            ASSERT_CONDITION(disableNagleAlgorithm(socketFd), "disableNagle() failed. errno: {}", std::string(strerror(errno)));
        }

        if (!socketConfig.isListeningMode) {
            ASSERT_CONDITION(connect(socketFd, rp->ai_addr, rp->ai_addrlen) == -1, "connect() failed. errno: {}", std::string(strerror(errno)));
        }

        if (socketConfig.isListeningMode) {
            ASSERT_CONDITION(setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&one), sizeof(one)) == 0,
                            "setsockopt() SO_REUSEADDR failed. errno: {} ", std::string(strerror(errno)));
        }
        if (socketConfig.isListeningMode) {

            sockaddr_in addr{ AF_INET ,{}, htonl(INADDR_ANY),{},{} };
            addr.sin_port = htons(socketConfig.portNumber);
            ASSERT_CONDITION(bind(socketFd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == 0,
                            "bind() failed. errno: {} ", std::string(strerror(errno)));
        }
        if (!socketConfig.useUdp && socketConfig.isListeningMode) {
            ASSERT_CONDITION(listen(socketFd, MaxTCPServerBacklog) == 0, "listen() failed. errno: {}", std::string(strerror(errno)));
        }

        if (socketConfig.enableTimestamp) {
            ASSERT_CONDITION(enableSocketTimestamp(socketFd), "setSOTimestamp() failed. errno: {} ", std::string(strerror(errno)));
        }

    }

    freeaddrinfo(result);

    return socketFd;
}

} // namespace lib

#endif // LOW_LATENCY_TRADING_APP_SOCKET_UTILS_H