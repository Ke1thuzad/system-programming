#include "TcpClient.hpp"

TcpClient::TcpClient() {
    TcpUtils::initializeNetwork();
}

TcpClient::~TcpClient() {
    disconnect();
    TcpUtils::shutdownNetwork();
}

void TcpClient::connect(const std::string& host, uint16_t port) {
    disconnect();

    _socket = std::make_unique<TcpSocket>();
    _socket->connect(host, port);
}

void TcpClient::disconnect() {
    if (_socket && _socket->isValid()) {
        _socket->close();
    }
    _socket.reset();
}

void TcpClient::send(const DataBuffer& data) {
    if (!isConnected()) {
        throw SocketConnectionException(0);
    }
    _socket->send(data);
}

DataBuffer TcpClient::receive() {
    if (!isConnected()) {
        throw SocketConnectionException(0);
    }
    return _socket->receiveAll();
}

std::string TcpClient::getRemoteAddress() const {
    return isConnected() ? TcpUtils::getAddressString(_socket->getHandle()) : "";
}

uint16_t TcpClient::getRemotePort() const {
    return isConnected() ? TcpUtils::getPort(_socket->getHandle()) : 0;
}