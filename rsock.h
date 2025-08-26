#pragma once

#include <functional>
#include <expected>
#include <string>

namespace rsock {
    enum class SendError { unknown };
    enum class RecvError { unknown };
}

namespace rsock::tcp {
    class Stream {
    public:
        Stream(const char* ip, short port);
        Stream(const std::string& ip, short port);
        ~Stream();

		std::expected<size_t, SendError> send(const std::vector<char>& data) const noexcept;
		std::expected<size_t, SendError> send(const std::string& data) const noexcept;
		std::expected<size_t, RecvError> recv(std::vector<char>& buff) const noexcept;

	  private:
		int sockfd;

		Stream(int sockfd);

        friend class Listener;
    };

    class Listener {
    public:
        Listener(const char* ip, short port);
        Listener(const std::string& ip, short port);
        ~Listener();

        void listen(const std::function<void(const Stream&)>& callback, int backlog) const;
    private:
        int sockfd;
    };
}
