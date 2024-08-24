#include "tcp_socket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "logger.h"

namespace utils {

TCPSocket::TCPSocket() noexcept {
    outboundData_.resize(TCPBufferSize);
    inboundData_.resize(TCPBufferSize);
}

TCPSocket::~TCPSocket() noexcept {
    if (socketFd_ != -1) {
        close(socketFd_);
    }
}

auto TCPSocket::connect(std::string_view ip, std::string_view iface, int port, bool isListening) -> int {
    const SocketConfig socketConfig{std::string(ip), std::string(iface), port, false, isListening, true};
    socketFd_ = createSocket(socketConfig);

    socketAttrib_.sin_addr.s_addr = INADDR_ANY;
    socketAttrib_.sin_port = htons(port);
    socketAttrib_.sin_family = AF_INET;

    return socketFd_;
}

auto TCPSocket::sendAndRecv() noexcept -> bool {
    char ctrl[CMSG_SPACE(sizeof(struct timeval))];
    auto* cmsg = reinterpret_cast<struct cmsghdr*>(&ctrl);

    iovec iov{inboundData_.data() + nextRcvValidIndex_, TCPBufferSize - nextRcvValidIndex_};
    msghdr msg{&socketAttrib_, sizeof(socketAttrib_), &iov, 1, ctrl, sizeof(ctrl), 0};

    const auto read_size = recvmsg(socketFd_, &msg, MSG_DONTWAIT);
    if (read_size > 0) {
        nextRcvValidIndex_ += read_size;

        Nanos kernel_time = 0;
        if (timeval time_kernel{};
            (cmsg->cmsg_level == SOL_SOCKET) && (cmsg->cmsg_type == SCM_TIMESTAMP) && (cmsg->cmsg_len == CMSG_LEN(sizeof(time_kernel)))) {
            std::memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
            kernel_time = time_kernel.tv_sec * NANOS_TO_SECS + time_kernel.tv_usec * NANOS_TO_MICROS;
        }

        LOG_INFOF("Received {} bytes from socket {}. Kernel time: {}", read_size, socketFd_, kernel_time);
        if (recvCallback_) {
            recvCallback_(this, kernel_time);
        }
    }

    if (nextSendValidIndex_ > 0) {
        const auto n = ::send(socketFd_, outboundData_.data(), nextSendValidIndex_, MSG_DONTWAIT | MSG_NOSIGNAL);
        LOG_INFOF("Sent {} bytes to socket {}", n, socketFd_);
        nextSendValidIndex_ = 0;
    }

    return (read_size > 0);
}

auto TCPSocket::send(std::span<const std::byte> data) noexcept -> void {
    std::memcpy(outboundData_.data() + nextSendValidIndex_, data.data(), data.size());
    nextSendValidIndex_ += data.size();
}

} // namespace utils