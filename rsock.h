#pragma once

#include <functional>
#include <expected>
#include <string>

namespace rsock {
enum class SendError {
    would_block,
    connection_reset,
    message_too_large,
    not_connected,
    unknown,
};

enum class RecvError {
    would_block,
    connection_reset,
    connection_timeout,
    not_enough_memory,
    unknown,
};
} // namespace rsock

namespace rsock::tcp {
class Stream {
public:
    Stream(const char* ip, short port);

    Stream(const std::string& ip, short port);

    Stream(const Stream& other) = delete;

    Stream(Stream&& other) noexcept;

    ~Stream();

    Stream& operator=(const Stream& other) = delete;

    Stream& operator=(Stream&& other) noexcept;


    std::expected<size_t, SendError> send(
        const std::vector<char>& data) const noexcept;

    std::expected<size_t, SendError> send(
        const std::string& data) const noexcept;

    std::expected<size_t, RecvError> recv(
        std::vector<char>& buff) const noexcept;

    int sockfd;

private:
    explicit Stream(int sockfd);

    friend class Listener;
};

class Listener {
public:
    Listener(const char* ip, short port);

    Listener(const std::string& ip, short port);

    Listener(const Listener& other) = delete;

    ~Listener();

    Listener& operator=(const Listener& other) = delete;

    void listen(const std::function<void(Stream&)>& callback) const;

private:
    int sockfd;
};
} // namespace rsock::tcp