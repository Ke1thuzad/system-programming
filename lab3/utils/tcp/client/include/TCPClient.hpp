#pragma once
#include "TcpCommon.hpp"
#include "Exceptions.hpp"

#include <string>
#include <memory>

class TcpClient {
public:
    TcpClient();
    virtual ~TcpClient();

    void connect(const std::string& host, uint16_t port);
    void disconnect();
    [[nodiscard]] bool isConnected() const { return _socket && _socket->isValid(); }

    void send(const DataBuffer& data);
    DataBuffer receive();

    [[nodiscard]] std::string getRemoteAddress() const;
    [[nodiscard]] uint16_t getRemotePort() const;

private:
    std::unique_ptr<TcpSocket> _socket;
};