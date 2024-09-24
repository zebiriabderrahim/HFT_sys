#include <gtest/gtest.h>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>
#include <random>

#include "lib/tcp_server.h"
#include "lib/socket_utils.h"


using namespace utils;

class TCPServerTest : public ::testing::Test {
  protected:
    TCPServer server;
#if defined(__linux__)
    std::string testInterface = "lo";
#elif defined(__APPLE__)
    std::string testInterface = "lo0";
#endif

    int testPort;

    void SetUp() override {
        testPort = getRandomPort();
    }

    void TearDown() override {
        server.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow time for port to be released
    }

    int getRandomPort() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(10000, 60000); // Range of unprivileged ports
        return dis(gen);
    }

    int createClientSocket() {
        SocketConfig config;
        config.ipAddress = "127.0.0.1";
        config.portNumber = testPort;
        config.useUdp = false;
        config.isListeningMode = false;

        int socketFd = createSocket(config);
        if (socketFd == -1) {
            ADD_FAILURE() << "Failed to create client socket. Error: " << strerror(errno);
        }
        return socketFd;
    }

    bool waitForConnection(int socketFd) {
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(socketFd, &write_fds);
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        if (select(socketFd + 1, NULL, &write_fds, NULL, &timeout) <= 0) {
            ADD_FAILURE() << "Connection timed out or select error. Error: " << strerror(errno);
            return false;
        }

        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            ADD_FAILURE() << "Connection failed after select. Error: " << strerror(error);
            return false;
        }

        return true;
    }
};

TEST_F(TCPServerTest, ListenAndStop) {
    ASSERT_NO_THROW(server.listen(testInterface, testPort)) << "Failed to start listening on " << testInterface << ":" << testPort;
    ASSERT_NO_THROW(server.stop()) << "Failed to stop the server";
}

TEST_F(TCPServerTest, SetCallbacks) {
    auto recvCallback = [](TCPSocket*, Nanos) {};
    auto recvFinishedCallback = []() {};

    ASSERT_NO_THROW(server.setRecvCallback(recvCallback)) << "Failed to set receive callback";
    ASSERT_NO_THROW(server.setRecvFinishedCallback(recvFinishedCallback)) << "Failed to set receive finished callback";
}

TEST_F(TCPServerTest, PollWithNoConnections) {
    ASSERT_NO_THROW(server.listen(testInterface, testPort)) << "Failed to start listening on " << testInterface << ":" << testPort;
    ASSERT_NO_THROW(server.poll()) << "Poll failed with no connections";
}

TEST_F(TCPServerTest, SendAndReceiveWithNoConnections) {
    ASSERT_NO_THROW(server.listen(testInterface, testPort)) << "Failed to start listening on " << testInterface << ":" << testPort;
    ASSERT_NO_THROW(server.sendAndReceive()) << "SendAndReceive failed with no connections";
}

TEST_F(TCPServerTest, AcceptConnection) {
    ASSERT_NO_THROW(server.listen(testInterface, testPort)) << "Failed to start listening on " << testInterface << ":" << testPort;

    int clientSocket = createClientSocket();
    ASSERT_NE(clientSocket, -1) << "Failed to create client socket";

    ASSERT_TRUE(waitForConnection(clientSocket)) << "Failed to connect client socket to server";

    // Give some time for the connection to be established
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Poll for the new connection
    ASSERT_NO_THROW(server.poll()) << "Poll failed after client connection";

    close(clientSocket);
}

TEST_F(TCPServerTest, ReceiveData) {
    bool dataReceived = false;
    server.setRecvCallback([&dataReceived](TCPSocket*, Nanos) {
        std::cout << "Receive callback invoked" << std::endl;
        dataReceived = true;
    });
    server.setRecvFinishedCallback([]() {
        std::cout << "Receive finished callback invoked" << std::endl;
    });

    ASSERT_NO_THROW(server.listen(testInterface, testPort)) << "Failed to start listening";

    int clientSocket = createClientSocket();
    ASSERT_NE(clientSocket, -1) << "Failed to create client socket";

    ASSERT_TRUE(waitForConnection(clientSocket)) << "Failed to connect client socket to server";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const char* testData = "Hello, Server!";
    ssize_t bytesSent = send(clientSocket, testData, strlen(testData), 0);
    ASSERT_NE(bytesSent, -1) << "Failed to send data. Error: " << strerror(errno);
    ASSERT_EQ(bytesSent, strlen(testData)) << "Not all data was sent";

    std::cout << "Data sent from client" << std::endl;

    ASSERT_NO_THROW(server.poll()) << "Poll failed after sending data";
    ASSERT_NO_THROW(server.sendAndReceive()) << "SendAndReceive failed after sending data";

    std::cout << "Server poll and sendAndReceive completed" << std::endl;

    ASSERT_TRUE(dataReceived) << "Data was not received by the server";

    close(clientSocket);
}
