#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "TcpCommon.hpp"
#include "Exceptions.hpp"

class TcpServer {
public:
    using ClientHandler = std::function<void(const DataBuffer&, socket_t)>;
    using ConnectionHandler = std::function<void(socket_t)>;

    TcpServer(uint16_t port, ClientHandler client_handler);
    virtual ~TcpServer();

    void start();
    void stop();
    bool isRunning() const { return _running; }

    void send(socket_t client, const DataBuffer& data);
    void disconnectClient(socket_t client);

    void setConnectionHandler(ConnectionHandler handler) { _connection_handler = std::move(handler); }
    void setDisconnectionHandler(ConnectionHandler handler) { _disconnection_handler = std::move(handler); }

private:
    void run();
    void handleClient(std::unique_ptr<TcpSocket> client_socket);

    uint16_t _port;

    ClientHandler _client_handler;
    ConnectionHandler _connection_handler;
    ConnectionHandler _disconnection_handler;

    std::unique_ptr<TcpSocket> _server_socket;
    std::thread _server_thread;
    std::mutex _clients_mutex;
    std::unordered_map<socket_t, std::thread> _client_threads;
    std::atomic<bool> _running{false};
};