#include <gtest/gtest.h>
#include <thread>

#include "lib/socket_utils.h"

using namespace utils;

class SocketUtilsTest : public ::testing::Test {
  protected:
    int socketFdTcp;
    int socketFdUdp;
#if defined(__linux__)
    std::string interfaceName = "lo"
#elif defined(__APPLE__)
    std::string interfaceName = "lo0";
#endif

    void SetUp() override {
        socketFdTcp = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_NE(socketFdTcp, -1) << "Failed to create TCP socket";

        socketFdUdp = socket(AF_INET, SOCK_DGRAM, 0);
        ASSERT_NE(socketFdUdp, -1) << "Failed to create UDP socket";
    }

    void TearDown() override {
        close(socketFdTcp);
        close(socketFdUdp);
    }
};

TEST_F(SocketUtilsTest, GetIpAddressForInterface) {

    std::string ip = getIpAddressForInterface(interfaceName);

    EXPECT_EQ(ip, "127.0.0.1") << "Loopback IP address not correctly retrieved";

    ip = getIpAddressForInterface("nonexistent");
    EXPECT_TRUE(ip.empty()) << "Non-existent interface should return empty string";
}

TEST_F(SocketUtilsTest, SetSocketNonBlocking) {
    EXPECT_TRUE(setSocketNonBlocking(socketFdTcp)) << "Failed to set TCP socket to non-blocking";

    int flags = fcntl(socketFdTcp, F_GETFL, 0);
    EXPECT_NE(flags & O_NONBLOCK, 0) << "TCP socket not set to non-blocking mode";
}

TEST_F(SocketUtilsTest, DisableNagleAlgorithm) {
    EXPECT_TRUE(disableNagleAlgorithm(socketFdTcp)) << "Failed to disable Nagle's algorithm";

    int flag = 0;
    socklen_t len = sizeof(flag);
    getsockopt(socketFdTcp, IPPROTO_TCP, TCP_NODELAY, (void*) &flag, &len);
    EXPECT_NE(flag, 0) << "Nagle's algorithm not disabled";
}

TEST_F(SocketUtilsTest, EnableSocketTimestamp) {
    EXPECT_TRUE(enableSocketTimestamp(socketFdTcp)) << "Failed to enable socket timestamp";

    int flag = 0;
    socklen_t len = sizeof(flag);
    getsockopt(socketFdTcp, SOL_SOCKET, SO_TIMESTAMP, &flag, &len);
    EXPECT_NE(flag, 0) << "Socket timestamp not enabled";
}

TEST_F(SocketUtilsTest, JoinMulticastGroup) {
    EXPECT_TRUE(joinMulticastGroup(socketFdUdp, "239.255.255.250")) << "Failed to join multicast group";
}

TEST_F(SocketUtilsTest, CreateTCPClientSocket) {
    SocketConfig config;
    config.ipAddress = "127.0.0.1";
    config.portNumber = 8080;
    config.useUdp = false;
    config.isListeningMode = false;

    int fd = createSocket(config);
    EXPECT_NE(fd, -1) << "Failed to create TCP client socket";
    close(fd);
}

TEST_F(SocketUtilsTest, CreateTCPServerSocket) {
    SocketConfig config;
    config.ipAddress = "127.0.0.1";
    config.portNumber = 8080;
    config.useUdp = false;
    config.isListeningMode = true;

    int fd = createSocket(config);
    EXPECT_NE(fd, -1) << "Failed to create TCP server socket";
    close(fd);
}

TEST_F(SocketUtilsTest, CreateSocketWithInterface) {
    SocketConfig config;
    config.interfaceName = interfaceName;
    config.portNumber = 8080;
    config.useUdp = false;
    config.isListeningMode = true;

    int fd = createSocket(config);
    EXPECT_NE(fd, -1) << "Failed to create socket with interface specification";
    close(fd);
}

TEST_F(SocketUtilsTest, CreateSocketWithTimestamp) {
    SocketConfig config;
    config.ipAddress = "127.0.0.1";
    config.portNumber = 8080;
    config.useUdp = false;
    config.isListeningMode = false;
    config.enableTimestamp = true;

    int fd = createSocket(config);
    EXPECT_NE(fd, -1) << "Failed to create socket with timestamp enabled";

    int flag = 0;
    socklen_t len = sizeof(flag);
    getsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &flag, &len);
    EXPECT_NE(flag, 0) << "Socket timestamp not enabled on created socket";

    close(fd);
}

