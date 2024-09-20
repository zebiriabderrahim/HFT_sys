#include "tcp_server.h"
#include "assertion.h"
#include "socket_utils.h"

#include <algorithm>
#include <cstring>

namespace utils {

TCPServer::~TCPServer() noexcept {
    stop();
}

auto TCPServer::stop() noexcept -> void {
    if (eventFd_ != -1) {
        close(eventFd_);
        eventFd_ = -1;
    }

    receiveSockets_.clear();
    sendSockets_.clear();
}

auto TCPServer::setRecvFinishedCallback(std::function<void()> callback) noexcept -> void {
    recvFinishedCallback_ = std::move(callback);
}

auto TCPServer::setRecvCallback(RecvCallback callback) noexcept -> void {
    recvCallback_ = std::move(callback);
}

auto TCPServer::listen(std::string_view interfaceName, int port) -> void {
#ifdef __linux__
    eventFd_ = epoll_create1(0);
#else
    eventFd_ = kqueue();
#endif
    ASSERT_CONDITION(listenerSocket_.connect("", interfaceName, port, true) >= 0, "Listener socket failed to connect. iface: {}  port: {} error: {}",
                     std::string(interfaceName), std::to_string(port), std::string(strerror(errno)));
    ASSERT_CONDITION(addSocketToEventSystem(&listenerSocket_), "Unable to add listener socket to event system. error: {}",
                     std::string(strerror(errno)));
}

auto TCPServer::poll() noexcept -> void {
    const int maxEvents = 1 + sendSockets_.size() + receiveSockets_.size();
#ifdef __linux__
    const int n = epoll_wait(eventFd_, events_, maxEvents, 0);
#else
    struct timespec timeout = {0, 0}; // Equivalent to 0 timeout in epoll_wait
    const int n = kevent(eventFd_, nullptr, 0, events_, maxEvents, &timeout);
#endif

    bool haveNewConnection = false;
    for (int i = 0; i < n; ++i) {
#ifdef __linux__
        const auto &event = events_[i];
        auto socket = reinterpret_cast<TCPSocket *>(event.data.ptr);
#else
        const auto &event = events_[i];
        auto socket = reinterpret_cast<TCPSocket *>(event.udata);
#endif
        int fd = socket->getSocketFd();

#ifdef __linux__
        if (event.events & EPOLLIN) {
#else
        if (event.filter == EVFILT_READ) {
#endif
            if (socket == &listenerSocket_) {
                LOG_INFO("Received EPOLLIN on listener socket:{}", fd);
                haveNewConnection = true;
                continue;
            }

            LOG_INFO("Received EPOLLIN on socket:{}", fd);
            std::input_iterator auto it = std::ranges::find_if(receiveSockets_.begin(), receiveSockets_.end(), [socket](const auto &s) { return s == socket; });
            if (it == receiveSockets_.end()) {
                receiveSockets_.push_back(socket);
            }
        }

#ifdef __linux__
        if (event.events & EPOLLOUT) {
#else
        if (event.filter == EVFILT_WRITE) {
#endif
            LOG_INFO("Received EPOLLOUT on socket:{}", fd);
            std::input_iterator auto it =
                std::ranges::find_if(sendSockets_.begin(), sendSockets_.end(), [socket](const auto &s) { return s == socket; });
            if (it == sendSockets_.end()) {
                sendSockets_.push_back(socket);
            }
        }

#ifdef __linux__
        if (event.events & (EPOLLERR | EPOLLHUP)) {
#else
        if (event.flags & (EV_EOF | EV_ERROR)) {
#endif
            LOG_INFO("Received EPOLLERR or EPOLLHUP on socket:{}", fd);
            std::input_iterator auto it =
                std::ranges::find_if(receiveSockets_.begin(), receiveSockets_.end(), [socket](const auto &s) { return s == socket; });
            if (it == receiveSockets_.end()) {
                receiveSockets_.push_back(socket);
            }
        }
    }

    // Accept a new connection, create a TCPSocket and add it to our containers.
    while (haveNewConnection) {
        LOG_INFO("Accepting new connection on listener socket:{}", listenerSocket_.getSocketFd());
        sockaddr_storage addr{};
        socklen_t addr_len = sizeof(addr);
        int fd = accept(listenerSocket_.getSocketFd(), reinterpret_cast<sockaddr *>(&addr), &addr_len);
        if (fd == -1)
            break;

        ASSERT_CONDITION(setSocketNonBlocking(fd) && disableNagleAlgorithm(fd), "Failed to set non-blocking or no-delay on socket: {}",fd);

        LOG_INFO("Accepted new connection on listener socket:{}. New socket:{}", listenerSocket_.getSocketFd(), fd);

        auto socket = socketPool_.allocate();
        socket->setSocketFd(fd);
        socket->setRecvCallback(recvCallback_);
        ASSERT_CONDITION(addSocketToEventSystem(socket), "Unable to add socket. error: {}", std::string(strerror(errno)));

        if (std::ranges::find(receiveSockets_.begin(), receiveSockets_.end(), socket) == receiveSockets_.end())
            receiveSockets_.push_back(socket);
    }
}

void TCPServer::sendAndReceive() noexcept {
    auto recv = false;

    std::ranges::for_each(receiveSockets_.begin(), receiveSockets_.end(), [&recv](auto socket) {
        recv |= socket->sendAndRecv();
    });

    if (recv) // There were some events and they have all been dispatched, inform listener.
        recvFinishedCallback_();

    std::ranges::for_each(sendSockets_.begin(), sendSockets_.end(), [](auto socket) {
        socket->sendAndRecv();
    });
}

auto TCPServer::addSocketToEventSystem(TCPSocket *socket) const noexcept -> bool {
#ifdef __linux__
    epoll_event ev{EPOLLIN | EPOLLOUT | EPOLLET, {.ptr = socket.get()}};
    return epoll_ctl(eventFd_, EPOLL_CTL_ADD, socket->getSocketFd(), &ev) == 0;
#else
    struct kevent ev[2];
    EV_SET(&ev[0], socket->getSocketFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, socket);
    EV_SET(&ev[1], socket->getSocketFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, socket);
    kevent(eventFd_, ev, 2, nullptr, 0, nullptr);
    return true;
#endif
}

} // namespace lib