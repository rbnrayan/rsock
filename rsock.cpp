#include "rsock.h"

#include <expected>
#include <format>
#include <functional>
#include <stdexcept>

// C - Sockets
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Throwable wrappers around C socket basic calls
// 	- only supports linux (for now?)
namespace csocket {

static inline bool is_valid(int sockfd) noexcept {
	return (sockfd > 0);
}

static int create_socket_or_throw() {
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!is_valid(sockfd))
		throw std::runtime_error("Cannot create socket");

	return sockfd;
}

static sockaddr_in sockaddr_from(const char* ip, short port) {
	sockaddr_in addr = {};
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
		std::string fmt = std::format("Cannot parse ip {}", ip);
		throw std::runtime_error(fmt);
	}
	return addr;
}


static void socket_bind_or_throw(int sockfd, const char* ip, short port) {
	sockaddr_in addr = sockaddr_from(ip, port);

	if (bind(sockfd, reinterpret_cast<sockaddr *>(&addr),
			 sizeof(addr)) < 0) {
		std::string fmt = std::format("Cannot bind socket to {}:{}", ip, port);
		throw std::runtime_error(fmt);
	}
} // namespace csocket

static void socket_connect_or_throw(int sockfd, const char* ip, short port) {
	sockaddr_in addr = sockaddr_from(ip, port);

	if (connect(sockfd, reinterpret_cast<sockaddr *>(&addr),
				sizeof(addr)) < 0) {
		std::string fmt =
			std::format("Cannot connect socket to {}:{}", ip, port);
		throw std::runtime_error(fmt);
	}
}

static void socket_listen_or_throw(int sockfd, short backlog) {
	if (listen(sockfd, backlog) < 0)
		throw std::runtime_error("Cannot open socket for listening");
}

static int socket_accept_or_throw(int sockfd) {
	sockaddr_in peer_addr = {};
	socklen_t peer_len = sizeof(peer_addr);

	int peerfd = accept(
		sockfd, reinterpret_cast<sockaddr *>(&peer_addr), &peer_len);
	if (!is_valid(peerfd))
		throw std::runtime_error("Cannot accept incomming connection");

	return peerfd;
}

static inline ssize_t socket_recv(int sockfd, char* buff, size_t len, int flags) noexcept {
	return recv(sockfd, buff, len, flags);
}

static inline ssize_t socket_send(int sockfd, const char* buff, size_t len, int flags) noexcept {
	return send(sockfd, buff, len, flags);
}

} // namespace csocket

namespace rsock::tcp {

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

Stream::Stream(int sockfd) : sockfd(sockfd) {}

Stream::~Stream() {
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
}

std::expected<size_t, RecvError> Stream::recv(std::vector<char>& buff) const noexcept {
	ssize_t bytes = csocket::socket_recv(sockfd, buff.data(), buff.size(), 0);

	if (bytes < 0)
		return std::unexpected(RecvError::unknown);

	return bytes;
}

std::expected<size_t, SendError> Stream::send(const std::vector<char>& data) const noexcept {
	ssize_t bytes_sent = csocket::socket_send(sockfd, data.data(), data.size(), 0);
	if (bytes_sent < 0)
		return std::unexpected(SendError::unknown);

	return bytes_sent;
}

std::expected<size_t, SendError> Stream::send(const std::string& data) const noexcept {
	ssize_t bytes_sent = csocket::socket_send(sockfd, data.data(), data.size(), 0);
	if (bytes_sent < 0)
		return std::unexpected(SendError::unknown);

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

void Listener::listen(const std::function<void(const Stream &)> &callback,
						   short backlog) const {
	csocket::socket_listen_or_throw(sockfd, backlog);

	while (true) {
		int peer_handle = csocket::socket_accept_or_throw(sockfd);

		Stream peer(peer_handle);
		callback(peer);
	}
}

} // namespace rsock