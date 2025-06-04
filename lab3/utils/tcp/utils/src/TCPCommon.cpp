#include "TcpCommon.hpp"
#include "Exceptions.hpp"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

TcpSocket::TcpSocket() : _socket(INVALID_SOCKET_VALUE) {
#ifdef _WIN32
    _socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_socket == INVALID_SOCKET_VALUE) {
        throw SocketInitException(WSAGetLastError());
    }
#else
    _socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0) {
        throw SocketInitException(errno);
    }
#endif
}

TcpSocket::TcpSocket(socket_t socket) : _socket(socket) {}

TcpSocket::~TcpSocket() {
    if (isValid()) {
        close();
    }
}

void TcpSocket::bind(uint16_t port) const {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
#ifdef _WIN32
        throw SocketBindException(WSAGetLastError());
#else
        throw SocketBindException(errno);
#endif
    }
}

void TcpSocket::listen(int backlog) const {
    if (::listen(_socket, backlog) != 0) {
#ifdef _WIN32
        throw SocketListenException(WSAGetLastError());
#else
        throw SocketListenException(errno);
#endif
    }
}

void TcpSocket::connect(const std::string& host, uint16_t port) const {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        throw SocketConnectionException(
#ifdef _WIN32
                WSAGetLastError()
#else
                errno
#endif
        );
    }

    if (::connect(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
#ifdef _WIN32
        throw SocketConnectionException(WSAGetLastError());
#else
        throw SocketConnectionException(errno);
#endif
    }
}

std::unique_ptr<TcpSocket> TcpSocket::accept() const {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    socket_t client_socket = ::accept(_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_socket == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        throw SocketAcceptException(WSAGetLastError());
#else
        throw SocketAcceptException(errno);
#endif
    }

    return std::make_unique<TcpSocket>(client_socket);
}

void TcpSocket::close() {
    if (!isValid()) return;

#ifdef _WIN32
    ::closesocket(_socket);
#else
    ::close(_socket);
#endif
    _socket = INVALID_SOCKET_VALUE;
}

void TcpSocket::send(const void* data, size_t size) const {
    size_t total_sent = 0;
    while (total_sent < size) {
        int flags = 0;
#ifndef _WIN32
        flags = MSG_NOSIGNAL;
#endif
        int sent = ::send(_socket, static_cast<const char*>(data) + total_sent, size - total_sent, flags);

        if (sent <= 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAECONNRESET) {
                throw SocketDisconnectedException();
            }
            throw SocketWriteException(err);
#else
            if (errno == EPIPE) {
                throw SocketDisconnectedException();
            }
            throw SocketWriteException(errno);
#endif
        }
        total_sent += sent;
    }
}

void TcpSocket::send(const DataBuffer& data) const {
    uint32_t net_size = htonl(static_cast<uint32_t>(data.size()));
    send(&net_size, sizeof(net_size));

    if (!data.empty()) {
        const char* ptr = reinterpret_cast<const char*>(data.data());
        size_t total_sent = 0;
        const size_t CHUNK_SIZE = 4096;

        while (total_sent < data.size()) {
            size_t to_send = std::min(CHUNK_SIZE, data.size() - total_sent);
            ssize_t sent = ::send(_socket, ptr + total_sent, to_send, MSG_NOSIGNAL);

            if (sent <= 0) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    throw SocketDisconnectedException();
                }
                throw SocketWriteException(errno);
            }
            total_sent += sent;
        }
    }
}

DataBuffer TcpSocket::receive(size_t size) const {
    DataBuffer buffer(size);
    size_t total_received = 0;

    while (total_received < size) {
        int received = ::recv(_socket, reinterpret_cast<char*>(buffer.data()) + total_received, size - total_received, 0);
        if (received <= 0) {
#ifdef _WIN32
            throw SocketReadException(WSAGetLastError());
#else
            throw SocketReadException(errno);
#endif
        }
        total_received += received;
    }

    return buffer;
}

DataBuffer TcpSocket::receiveAll() {
    uint32_t net_size;
    ssize_t received = ::recv(_socket, reinterpret_cast<char*>(&net_size), sizeof(net_size), MSG_WAITALL);

    if (received == 0) {
        throw SocketDisconnectedException();
    } else if (received < 0) {
        if (errno == ECONNRESET) {
            throw SocketDisconnectedException();
        }
        throw SocketReadException(errno);
    }

    uint32_t size = ntohl(net_size);

    if (size > 0) {
        DataBuffer buffer(size);
        received = ::recv(_socket, reinterpret_cast<char*>(buffer.data()), size, MSG_WAITALL);

        if (received == 0) {
            throw SocketDisconnectedException();
        } else if (received < 0 || static_cast<uint32_t>(received) != size) {
            if (errno == ECONNRESET) {
                throw SocketDisconnectedException();
            }
            throw SocketReadException(errno);
        }
        return buffer;
    }
    return {};
}

void TcpSocket::setNonBlocking(bool non_blocking) const {
#ifdef _WIN32
    u_long mode = non_blocking ? 1 : 0;
    if (ioctlsocket(_socket, FIONBIO, &mode) != 0) {
        throw SocketOptionException(WSAGetLastError());
    }
#else
    int flags = fcntl(_socket, F_GETFL, 0);
    if (flags < 0) {
        throw SocketOptionException(errno);
    }
    flags = non_blocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(_socket, F_SETFL, flags) != 0) {
        throw SocketOptionException(errno);
    }
#endif
}

void TcpSocket::setReuseAddress(bool reuse) const {
    int optval = reuse ? 1 : 0;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&optval), sizeof(optval)) != 0) {
#ifdef _WIN32
        throw SocketOptionException(WSAGetLastError());
#else
        throw SocketOptionException(errno);
#endif
    }
}

void TcpUtils::initializeNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw SocketInitException(WSAGetLastError());
    }
#endif
}

void TcpUtils::shutdownNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

std::string TcpUtils::getAddressString(socket_t socket) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(socket, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        return "unknown";
    }

    char buffer[INET_ADDRSTRLEN];
    const char* result = inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer));
    return result ? result : "unknown";
}

uint16_t TcpUtils::getPort(socket_t socket) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(socket, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        return 0;
    }
    return ntohs(addr.sin_port);
}