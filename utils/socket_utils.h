//
// Created by ABDERRAHIM ZEBIRI on 2024-08-22.
//

//
// Created by ABDERRAHIM ZEBIRI on 2024-08-22.
//

#ifndef LOW_LATENCY_TRADING_APP_SOCKET_UTILS_H
#define LOW_LATENCY_TRADING_APP_SOCKET_UTILS_H
#include "debug_assertion.h"

#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <arpa/inet.h>

#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace utils {

/**
 * @brief Configuration for socket settings.
 */
struct SocketConfig {
    std::string ipAddress;                        ///< IP address of the socket.
    std::string interfaceName;                    ///< Network interface name.
    std::optional<int> portNumber = std::nullopt; ///< Port number, optional.
    bool useUdp = false;                          ///< Flag indicating if UDP protocol is used.
    bool isListeningMode = false;                 ///< Flag indicating if the socket is in listening mode.
    bool enableTimestamp = false;                 ///< Flag for enabling SO_TIMESTAMP option.

    /**
     * @brief Convert the socket configuration to a string representation.
     * @return A string detailing the socket configuration.
     */
    [[nodiscard]] auto toString() const -> std::string {
        return std::format("Socket_Config[ipAddress:{} interfaceName:{} portNumber:{} useUdp:{} isListeningMode:{} enableTimestamp:{}]", ipAddress,
                           interfaceName, portNumber.has_value() ? std::to_string(portNumber.value()) : "N/A", useUdp, isListeningMode,
                           enableTimestamp);
    }
};

/**
 * @brief Get the IP address of a network interface.
 * @param interfaceName The name of the network interface.
 * @return The IP address as a string.
 */
[[nodiscard]] inline auto getIpAddressForInterface(std::string_view interfaceName) -> std::string {
    char buf[NI_MAXHOST] = {'\0'};

    struct ifaddrs *ifaddr = nullptr;

    // Get the list of network interfaces
    if (getifaddrs(&ifaddr) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to get network interfaces");
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        // Check for IPv4 address and matching interface name
        if (ifa->ifa_addr->sa_family == AF_INET && interfaceName == ifa->ifa_name) {
            if (int result = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST); result != 0) {
                freeifaddrs(ifaddr);
                throw std::system_error(result, std::generic_category(), "Failed to get IP address");
            }
            break;
        }
    }

    freeifaddrs(ifaddr);

    assertCondition(buf[0] != '\0', "No matching interface found or interface has no IPv4 address");

    return std::string{buf};
}

/**
 * @brief Set a socket to non-blocking mode.
 * @param socketFd The file descriptor of the socket.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto setSocketNonBlocking(int socketFd) -> bool {
    const auto flags = fcntl(socketFd, F_GETFL, 0);
    if (flags == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to get file descriptor flags");
    }
    if (flags & O_NONBLOCK) {
        return true;
    }
    if (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to set file descriptor to non-blocking");
    }

    return false;
}

/**
 * @brief Disable the Nagle algorithm for a socket.
 * @param socketFd The file descriptor of the socket.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto disableNagleAlgorithm(int socketFd) -> bool {
    if (int flag = 1; setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to disable Nagle algorithm");
    }

    return true;
}

/**
 * @brief Enable SO_TIMESTAMP on a socket.
 * @param socketFd The file descriptor of the socket.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto enableSocketTimestamp(int socketFd) -> bool {
    if (int flag = 1; setsockopt(socketFd, SOL_SOCKET, SO_TIMESTAMP, &flag, sizeof(int)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to enable SO_TIMESTAMP");
    }

    return true;
}

/**
 * @brief Check if the last operation would block.
 * @return True if it would block, false otherwise.
 */
[[nodiscard]] inline auto isOperationWouldBlock() -> bool {
    return errno == EWOULDBLOCK || errno == EAGAIN;
}

/**
 * @brief Set the multicast Time-To-Live (TTL) for a socket.
 * @param socketFd The file descriptor of the socket.
 * @param ttlValue The TTL value.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto setMulticastTTL(int socketFd, int ttlValue) -> bool {
    if (setsockopt(socketFd, IPPROTO_IP, IP_MULTICAST_TTL, &ttlValue, sizeof(int)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to set multicast TTL");
    }

    return true;
}

/**
 * @brief Set the Time-To-Live (TTL) for a socket.
 * @param socketFd The file descriptor of the socket.
 * @param ttlValue The TTL value.
 * @return True if successful, false otherwise.
 */
[[nodiscard]] inline auto setSocketTTL(int socketFd, int ttlValue) -> bool {
    if (setsockopt(socketFd, IPPROTO_IP, IP_TTL, &ttlValue, sizeof(int)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to set socket TTL");
    }

    return true;
}

/**
 * @brief Join a multicast group on the specified interface.
 * @param fd The file descriptor of the socket.
 * @param multicastIp The IP address of the multicast group to join.
 * @param interfaceIp The IP address of the local interface to use (optional, defaults to INADDR_ANY).
 * @return True if successfully joined the multicast group.
 * @throws std::system_error if an error occurs while joining the multicast group.
 */
inline auto joinMulticastGroup(int fd, const std::string &multicastIp, const std::string &interfaceIp = "0.0.0.0") -> bool {
    struct ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(multicastIp.c_str());
    mreq.imr_interface.s_addr = inet_addr(interfaceIp.c_str());

    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to join multicast group");
    }

    return true;
}

/**
 * @brief Create a socket based on the configuration.
 * @param targetIp The target IP address.
 * @param socketConfig The socket configuration.
 * @return The file descriptor of the created socket.
 */
[[nodiscard]] inline auto createSocket(std::string_view targetIp, SocketConfig &socketConfig) -> int;
} // namespace utils

#endif // LOW_LATENCY_TRADING_APP_SOCKET_UTILS_H