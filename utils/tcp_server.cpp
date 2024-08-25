#include "tcp_server.h"
#include "debug_assertion.h"
#include "socket_utils.h"
#include <algorithm>

namespace utils {

TCPServer::~TCPServer() {
    stop();
}

bool TCPServer::listen(std::string_view interfaceName, int port) {
#ifdef __linux__
    eventFd_ = epoll_create1(0);
#else
    eventFd_ = kqueue();
#endif
    assertCondition(eventFd_ >= 0, "Failed to create event system. Error: " + std::string(strerror(errno)));

    SocketConfig config;
    config.interfaceName = std::string(interfaceName);
    config.portNumber = port;
    config.isListeningMode = true;
    config.enableTimestamp = true;

    int socketFd = createSocket(config);
    assertCondition(socketFd != -1, "Failed to create listener socket");

    listenerSocket_ = std::make_unique<TCPSocket>();
    return addSocketToEventSystem(listenerSocket_.get());
}

auto TCPServer::poll() noexcept -> void {
    const auto maxEvents = static_cast<int>(1 + sendSockets_.size() + receiveSockets_.size());
#ifdef __linux__
    const int n = epoll_wait(eventFd_, events_, maxEvents, 0);
#else
    const int n = kevent(eventFd_, nullptr, 0, events_, maxEvents, nullptr);
#endif

    bool have_new_connection = false;
    for (int i = 0; i < n; ++i) {
#ifdef __linux__
        const auto &event = events_[i];
        auto socket = reinterpret_cast<TCPSocket *>(event.data.ptr);
#else
        const auto &event = events_[i];
        auto socket = reinterpret_cast<TCPSocket *>(event.udata);
#endif

#ifdef __linux__
        if (event.events & EPOLLIN) {
#else
        if (event.filter == EVFILT_READ) {
#endif
            if (socket == listenerSocket_.get()) {
                LOG_INFOF("Received EPOLLIN on listener socket:%d", socket->getSocketFd());
                have_new_connection = true;
                continue;
            }

            LOG_INFOF("Received EPOLLIN on socket:%d", socket->getSocketFd());
            auto it = std::ranges::find_if(receiveSockets_.begin(), receiveSockets_.end(), [socket](const auto &s) { return s.get() == socket; });
            if (it == receiveSockets_.end()) {
                receiveSockets_.push_back(std::unique_ptr<TCPSocket>(socket));
            }
        }

#ifdef __linux__
        if (event.events & EPOLLOUT) {
#else
        if (event.filter == EVFILT_WRITE) {
#endif
            LOG_INFOF("Received EPOLLOUT on socket:%d", socket->getSocketFd());
            auto it = std::ranges::find_if(sendSockets_.begin(), sendSockets_.end(), [socket](const auto &s) { return s.get() == socket; });
            if (it == sendSockets_.end()) {
                sendSockets_.push_back(std::unique_ptr<TCPSocket>(socket));
            }
        }

#ifdef __linux__
        if (event.events & (EPOLLERR | EPOLLHUP)) {
#else
        if (event.flags & (EV_EOF | EV_ERROR)) {
#endif
            LOG_INFOF("Received EPOLLERR or EPOLLHUP on socket:%d", socket->getSocketFd());
            auto it = std::ranges::find_if(receiveSockets_.begin(), receiveSockets_.end(), [socket](const auto &s)
                                   { return s.get() == socket; });
            if (it != receiveSockets_.end()) {
                receiveSockets_.erase(it);
            }
        }
    }

    // Accept a new connection, create a TCPSocket and add it to our containers.
    while (have_new_connection) {
        LOG_INFOF("Accepting new connection on listener socket:%d", listenerSocket_->getSocketFd());
        sockaddr_storage addr{};
        socklen_t addr_len = sizeof(addr);
        int fd = accept(listenerSocket_->getSocketFd(), reinterpret_cast<sockaddr *>(&addr), &addr_len);
        if (fd == -1)
            break;

        assertCondition(setSocketNonBlocking(fd) && disableNagleAlgorithm(fd),
                        "Failed to set non-blocking or no-delay on socket:" + std::to_string(fd));

        LOG_INFOF("Accepted new connection on listener socket:%d. New socket:%d", listenerSocket_->getSocketFd(), fd);

        auto socket = std::make_unique<TCPSocket>();
        socket->setSocketFd(fd);
        socket->setRecvCallback(recvCallback_);
        assertCondition(addSocketToEventSystem(socket.get()), "Unable to add socket. error:" + std::string(strerror(errno)));

        if (std::ranges::find(receiveSockets_.begin(), receiveSockets_.end(), socket) == receiveSockets_.end())
            receiveSockets_.push_back(std::move(socket));
    }
}

void TCPServer::sendAndReceive() noexcept {
    bool receivedData = false;

    for (const auto &socket : receiveSockets_) {
        if (socket->sendAndRecv()) {
            receivedData = true;
            if (recvCallback_) {
                recvCallback_(socket.get(), getCurrentNanos());
            }
        }
    }

    if (receivedData && recvFinishedCallback_) {
        recvFinishedCallback_();
    }

    for (const auto &socket : sendSockets_) {
        socket->sendAndRecv();
    }
}

void TCPServer::stop() noexcept {
    receiveSockets_.clear();
    sendSockets_.clear();

    if (eventFd_ != -1) {
        close(eventFd_);
        eventFd_ = -1;
    }
}
auto TCPServer::addSocketToEventSystem(TCPSocket* socket) const noexcept -> bool {
#ifdef __linux__
    epoll_event ev{EPOLLIN | EPOLLOUT | EPOLLET, {.ptr = socket.get()}};
    return epoll_ctl(eventFd_, EPOLL_CTL_ADD, socket->getSocketFd(), &ev) == 0;
#else
    struct kevent ev[2];
    EV_SET(&ev[0], socket->getSocketFd(), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, socket);
    EV_SET(&ev[1], socket->getSocketFd(), EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, socket);
    return kevent(eventFd_, ev, 2, nullptr, 0, nullptr) != -1;
#endif
}

} // namespace utils