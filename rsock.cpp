#include "rsock.h"

#include <expected>
#include <format>
#include <functional>
#include <stdexcept>

// C - Sockets
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

// Throwable wrappers around C socket basic calls
// 	- only supports linux (for now?)
namespace csocket {

inline bool is_valid(int sockfd) noexcept {
    return (sockfd > 0);
}

int create_socket_or_throw() {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!is_valid(sockfd))
        throw std::runtime_error("Cannot create socket");

    return sockfd;
}

sockaddr_in sockaddr_from(const char* ip, short port) {
    sockaddr_in addr = {};
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        std::string fmt = std::format("Cannot parse ip {}", ip);
        throw std::runtime_error(fmt);
    }
    return addr;
}


void socket_bind_or_throw(int sockfd, const char* ip, short port) {
    sockaddr_in addr = sockaddr_from(ip, port);

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) < 0) {
        std::string fmt = std::format("Cannot bind socket to {}:{}", ip, port);
        throw std::runtime_error(fmt);
    }
} // namespace csocket

void socket_connect_or_throw(int sockfd, const char* ip, short port) {
    sockaddr_in addr = sockaddr_from(ip, port);

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&addr),
                sizeof(addr)) < 0) {
        std::string fmt =
            std::format("Cannot connect socket to {}:{}", ip, port);
        throw std::runtime_error(fmt);
    }
}

void socket_listen_or_throw(int sockfd, int backlog) {
    if (listen(sockfd, backlog) < 0)
        throw std::runtime_error("Cannot open socket for listening");
}

int socket_accept_or_throw(int sockfd) {
    sockaddr_in peer_addr = {};
    socklen_t peer_len = sizeof(peer_addr);

    int peerfd = accept(
        sockfd, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
    if (!is_valid(peerfd))
        throw std::runtime_error("Cannot accept incomming connection");

    return peerfd;
}

inline ssize_t socket_recv(int sockfd, char* buff, size_t len,
                                  int flags) noexcept {
    return recv(sockfd, buff, len, flags);
}

inline ssize_t socket_send(int sockfd, const char* buff, size_t len,
                                  int flags) noexcept {
    return send(sockfd, buff, len, flags);
}

} // namespace csocket

namespace rsock::tcp {

const int DEFAULT_BACKLOG_SIZE = 128;

Stream::Stream(const char* ip, short port) {
    int sockfd = csocket::create_socket_or_throw();
    csocket::socket_connect_or_throw(sockfd, ip, port);

    this->sockfd = sockfd;
}

Stream::Stream(const std::string& ip, short port) {
    int sockfd = csocket::create_socket_or_throw();
    csocket::socket_connect_or_throw(sockfd, ip.c_str(), port);

    this->sockfd = sockfd;
}

Stream::Stream(int sockfd) : sockfd(sockfd) {
}

Stream::Stream(Stream&& other) noexcept : sockfd(other.sockfd) {
    other.sockfd = -1;
}

Stream& Stream::operator=(Stream&& other) noexcept {
    this->sockfd = other.sockfd;
    other.sockfd = -1;

    return *this;
}

Stream::~Stream() {
    if (sockfd < 0)
        return;

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
}

RecvError recv_error_from_errno() {
    switch (errno) {
    case EAGAIN | EWOULDBLOCK:
        return RecvError::would_block;
    case ECONNRESET:
        return RecvError::connection_reset;
    case ETIMEDOUT:
        return RecvError::connection_timeout;
    case ENOBUFS:
        return RecvError::not_enough_memory;
    default:
        return RecvError::unknown;
    }
}

std::expected<size_t, RecvError> Stream::recv(
    std::vector<char>& buff) const noexcept {
    ssize_t bytes = csocket::socket_recv(sockfd, buff.data(), buff.size(), 0);

    if (bytes < 0) {
        RecvError unexpected_value = recv_error_from_errno();
        return std::unexpected(unexpected_value);
    }

    return bytes;
}

SendError send_error_from_errno() {
    switch (errno) {
    case EAGAIN | EWOULDBLOCK:
        return SendError::would_block;
    case ECONNRESET:
        return SendError::connection_reset;
    case EMSGSIZE:
        return SendError::message_too_large;
    case ENOTCONN:
        return SendError::not_connected;
    default:
        return SendError::unknown;
    }
}

std::expected<size_t, SendError> Stream::send(
    const std::vector<char>& data) const noexcept {
    ssize_t bytes_sent = csocket::socket_send(sockfd, data.data(), data.size(),
                                              0);
    if (bytes_sent < 0) {
        SendError unexpected_value = send_error_from_errno();
        return std::unexpected(unexpected_value);
    }

    return bytes_sent;
}

std::expected<size_t, SendError> Stream::send(
    const std::string& data) const noexcept {
    ssize_t bytes_sent = csocket::socket_send(sockfd, data.data(), data.size(),
                                              0);
    if (bytes_sent < 0) {
        SendError unexpected_value = send_error_from_errno();
        return std::unexpected(unexpected_value);
    }

    return bytes_sent;
}

Listener::Listener(const char* ip, short port) {
    int sockfd = csocket::create_socket_or_throw();
    csocket::socket_bind_or_throw(sockfd, ip, port);

    this->sockfd = sockfd;
}

Listener::Listener(const std::string& ip, short port) {
    int sockfd = csocket::create_socket_or_throw();
    csocket::socket_bind_or_throw(sockfd, ip.c_str(), port);

    this->sockfd = sockfd;
}

Listener::~Listener() {
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
}

void Listener::listen(const std::function<void(Stream&)>& callback) const {
    csocket::socket_listen_or_throw(sockfd, DEFAULT_BACKLOG_SIZE);

    while (true) {
        int peer_handle = csocket::socket_accept_or_throw(sockfd);

        Stream peer(peer_handle);
        callback(peer);
    }
}
} // namespace rsock
