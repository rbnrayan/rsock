# RSOCK - RAII Sockets

Very simple library that wraps C sockets with `tcp::Listener` and `tcp::Stream`.

## Exemple usage

### `Listener.cpp`

```c++
#include <print>

#include "rsock.h"

const std::string IP = "0.0.0.0";
const std::uint16_t PORT = 12345;

using namespace rsock;

int main() {
    tcp::Listener listener(IP, PORT);

    listener.listen([](tcp::Stream& peer) {
        std::expected<std::size_t, SendError> bytes_sent =
            peer.send("Hello world!");

        if (bytes_sent)
            std::println("Sent {} bytes to peer",
                         bytes_sent.value());
        else {}
            // Check for rsock::SendError here...
    });

    // Unreachable
    return EXIT_SUCCESS;
}
```

### `Client.cpp`
```c++
#include <print>

#include "rsock.h"

const std::string IP = "127.0.0.1";
const std::uint16_t PORT = 12345;

using namespace rsock;

int main() {
    tcp::Stream stream(IP, PORT);

    // Need to initialize a size to receive
    std::vector<char> buff(1024);
    std::expected<std::size_t, RecvError> recv_bytes =
        stream.recv(buff);

    if (recv_bytes) {
        std::string recv_value(buff.begin(), buff.end());
        std::println("Received {} bytes: {}",
                     recv_bytes.value(), recv_value);
    } else {
        // Check for rsock::RecvError here...
    }

    return EXIT_SUCCESS;
}
```