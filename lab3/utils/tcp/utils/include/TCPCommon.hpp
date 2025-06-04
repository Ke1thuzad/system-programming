#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
using socket_t = int64_t;
constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

using DataBuffer = std::vector<uint8_t>;

class TcpSocket {
    socket_t _socket;
public:
    TcpSocket();
    explicit TcpSocket(socket_t socket);
    virtual ~TcpSocket();

    void bind(uint16_t port) const;
    void listen(int backlog = SOMAXCONN) const;
    void connect(const std::string& host, uint16_t port) const;
    [[nodiscard]] std::unique_ptr<TcpSocket> accept() const;
    void close();

    void send(const void* data, size_t size) const;
    void send(const DataBuffer& data) const;
    [[nodiscard]] DataBuffer receive(size_t size) const;
    DataBuffer receiveAll();

    void setNonBlocking(bool non_blocking) const;
    void setReuseAddress(bool reuse) const;

    [[nodiscard]] socket_t getHandle() const { return _socket; }
    [[nodiscard]] bool isValid() const { return _socket != INVALID_SOCKET_VALUE; }
};

namespace TcpUtils {
    void initializeNetwork();
    void shutdownNetwork();
    std::string getAddressString(socket_t socket);
    uint16_t getPort(socket_t socket);
}