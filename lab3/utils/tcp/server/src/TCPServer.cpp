#include "TcpServer.hpp"

TcpServer::TcpServer(uint16_t port, ClientHandler client_handler)
        : _port(port), _client_handler(std::move(client_handler)) {}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::start() {
    if (_running) return;

    try {
        TcpUtils::initializeNetwork();

        _server_socket = std::make_unique<TcpSocket>();
        _server_socket->setReuseAddress(true);
        _server_socket->bind(_port);
        _server_socket->listen();

        _running = true;
        _server_thread = std::thread(&TcpServer::run, this);
    } catch (const std::exception& e) {
        throw ServerException("Failed to start server: " + std::string(e.what()));
    }
}

void TcpServer::stop() {
    if (!_running) return;

    _running = false;

    if (_server_socket && _server_socket->isValid()) {
        _server_socket->close();
    }

    if (_server_thread.joinable()) {
        _server_thread.join();
    }

    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        for (auto& [client, thread] : _client_threads) {
            disconnectClient(client);
        }
    }

    TcpUtils::shutdownNetwork();
}

void TcpServer::run() {
    while (_running) {
        try {
            auto client_socket = _server_socket->accept();
            socket_t client_handle = client_socket->getHandle();

            if (_connection_handler) {
                _connection_handler(client_handle);
            }

            {
                std::lock_guard<std::mutex> lock(_clients_mutex);
                _client_threads.emplace(
                        client_handle,
                        std::thread(&TcpServer::handleClient, this, std::move(client_socket))
                );
            }
        } catch (const SocketAcceptException& e) {
            if (_running) {
                std::cerr << "Accept error: " << e.what() << std::endl;
            }
        }
    }
}

void TcpServer::handleClient(std::unique_ptr<TcpSocket> client_socket) {
    socket_t client_handle = client_socket->getHandle();

    try {
        while (_running) {
            try {
                DataBuffer data = client_socket->receiveAll();
                _client_handler(data, client_handle);
            }
            catch (const SocketDisconnectedException&) {
                break;
            }
        }
    } catch (...) {}

    if (_disconnection_handler) {
        _disconnection_handler(client_handle);
    }

    client_socket->close();

    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        if (auto it = _client_threads.find(client_handle); it != _client_threads.end()) {
            if (it->second.joinable()) {
                it->second.detach();
            }
            _client_threads.erase(it);
        }
    }
}

void TcpServer::send(socket_t client, const DataBuffer& data) {
    std::lock_guard<std::mutex> lock(_clients_mutex);
    auto it = _client_threads.find(client);
    if (it != _client_threads.end()) {
        TcpSocket socket(client);
        socket.send(data);
    }
}

void TcpServer::disconnectClient(socket_t client) {
    std::lock_guard<std::mutex> lock(_clients_mutex);
    auto it = _client_threads.find(client);
    if (it != _client_threads.end()) {
        TcpSocket socket(client);
        socket.close();

        if (it->second.joinable()) {
            it->second.join();
        }
        _client_threads.erase(it);
    }
}