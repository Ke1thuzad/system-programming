#include "Server.hpp"

Server::Server(const char* token_path, int proj_id, uint16_t port, LogLevels level)
        : _message_queue(token_path, proj_id),
          _shared_memory(token_path, proj_id, MAX_FILE_SIZE),
          _clients_sem(token_path, proj_id, 1),
          _is_running(false) {

    _logger = Logger::Builder().SetPrefix("server").SetLogLevel(level).Build();

    _server = std::make_unique<TcpServer>(port, [this](const DataBuffer& buffer, socket_t sock) {
        HandleClientData(buffer, sock);
    });

    _server->setConnectionHandler([this](socket_t sock) {
        _logger->LogInfo(std::format("Client {} connected", sock));

        Semaphore::Guard lock(_clients_sem, 0);
        _connected_clients[sock] = {std::make_shared<TcpSocket>(sock), MAIN_MENU};
    });

    _server->setDisconnectionHandler([this](socket_t sock) {
        _logger->LogInfo(std::format("Client {} disconnected", sock));

        auto it = _connected_clients.find(sock);

        Semaphore::Guard lock(_clients_sem, 0);

        if (it->second._current_state == GAME) {
            try {
                _message_queue.send(sock, -1, CLIENT_CONNECT);
            } catch (const IPCException& e) {
                _logger->LogError(e.what());
            }
        }

        _connected_clients.erase(it);
    });
}

Server::~Server() {
    Stop();
}

void Server::Start() {
    if (_is_running) return;

    _is_running = true;
    _server->start();
    _logger->LogInfo("Server started");
}

void Server::Stop() {
    if (!_is_running) return;

    _is_running = false;
    _server->stop();
    _logger->LogInfo("Server stopped");
}

void Server::HandleClientData(const DataBuffer& buffer, socket_t sock) {
    std::map<socket_t, Server::ClientConnection>::iterator it;
    {
        Semaphore::Guard lock(_clients_sem, 0);
        it = _connected_clients.find(sock);
        if (it == _connected_clients.end()) {
            _logger->LogWarning("Received data from disconnected client: " + std::to_string(sock));
            return;
        }
    }

    ClientConnection &clientConnection = it->second;

    if (buffer.empty()) {
        _logger->LogWarning("Empty buffer from client " + std::to_string(sock));
        return;
    }

    try {
        uint8_t command = buffer[0];

        if (clientConnection._current_state == GAME) {
            if (std::isdigit(command))
                HandleGameMove(sock, command - '0', clientConnection);
            else
                throw ServerException("Invalid command");
            return;
        }

        switch (command) {
            case COMPILE_COMMAND:
                HandleCompilerRequest(buffer, sock, clientConnection);
                break;
            case GAME_COMMAND:
                HandleGameStart(sock, clientConnection);
                break;
            case DISCONNECT_COMMAND:
                _server->disconnectClient(sock);
                break;
            default:
                throw ServerException("Invalid command");
        }
    }
    catch (const SocketDisconnectedException&) {
        _logger->LogInfo("Client disconnected during processing: " + std::to_string(sock));
        _server->disconnectClient(sock);
    }
    catch (const std::exception& e) {
        try {
            DataBuffer response{ERROR_RESPONSE};
            std::string err_msg = "Error: " + std::string(e.what());
            response.insert(response.end(), err_msg.begin(), err_msg.end());
            clientConnection.socket->send(response);
        }
        catch (const SocketException& send_err) {
            _logger->LogError("Failed to send error to client " + std::to_string(sock) +
                              ": " + send_err.what());
        }

        _logger->LogError(std::format("Error handling client {}: {}", sock, e.what()));
    }
}

void Server::HandleMainMenu(socket_t sock, int command, ClientConnection& clientConnection) {
    command -= '0';

    std::string response;

    switch (command) {
        case 0:
            clientConnection._current_state = COMPILER;

            response = "Compiler is waiting for a file";

            clientConnection.socket->send({response.begin(), response.end()});
            break;
        case 1:
            clientConnection._current_state = GAME;
            _message_queue.send(sock, 1, CLIENT_CONNECT);

            response = "A sticks game has been started";

            clientConnection.socket->send({response.begin(), response.end()});
            break;
        case 2:
            Stop();
            break;
        default:
            throw ServerException("Invalid menu command: " + std::to_string(command));
    }
}

void Server::HandleGameStart(socket_t sock, ClientConnection& clientConnection) {
    clientConnection._current_state = GAME;
    _message_queue.send(sock, 1, CLIENT_CONNECT);


    DataBuffer response{GAME_START};
    std::string msg = "Game started! You have 15 sticks. Take 1-3 sticks each turn.";
    response.insert(response.end(), msg.begin(), msg.end());

    clientConnection.socket->send(response);

    _logger->LogInfo("Game started for client " + std::to_string(sock));
}

void Server::HandleCompilerRequest(const DataBuffer& buffer, socket_t sock, ClientConnection& clientConnection) {
    if (buffer.size() <= 1) {
        throw ServerException("Invalid file data");
    }

    auto null_pos = std::find(buffer.begin() + 1, buffer.end(), '\0');
    if (null_pos == buffer.end()) {
        throw ServerException("Invalid file format: missing filename separator");
    }

    std::string filename_with_path(buffer.begin() + 1, null_pos);
    std::filesystem::path full_path(filename_with_path);
    std::string filename = full_path.filename().string();

    std::vector<uint8_t> file_data(null_pos + 1, buffer.end());

    std::filesystem::path upload_dir = "./uploads";
    if (!std::filesystem::exists(upload_dir)) {
        if (!std::filesystem::create_directories(upload_dir)) {
            throw ServerException("Failed to create upload directory");
        }
    }

    std::filesystem::path client_file = upload_dir / filename;
    {
        std::ofstream file(client_file, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            throw ServerException("Failed to open file for writing: " + client_file.string());
        }
        file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        if (!file.good()) {
            throw ServerException("Failed to write file: " + client_file.string());
        }
    }

    ushort filename_size = filename.size() + 1;
    uint32_t file_size = file_data.size();
    int64_t user_id = sock;

    uint8_t flag = 1;

    _shared_memory.block();

    _shared_memory.clear();
    _shared_memory.write(std::span<const char>(reinterpret_cast<const char*>(&flag), 1));
    _shared_memory.write(std::span<const char>(reinterpret_cast<const char*>(&user_id), sizeof(int64_t)));
    _shared_memory.write(std::span<const char>(reinterpret_cast<const char*>(&filename_size), sizeof(filename_size)));
    _shared_memory.write(std::span<const char>(filename.data(), filename_size));
    _shared_memory.write(std::span<const char>(reinterpret_cast<const char*>(&file_size), sizeof(file_size)));
    _shared_memory.write(std::span<const char>(reinterpret_cast<const char*>(file_data.data()), file_size));

    _shared_memory.unblock();

    while (!_shared_memory.readyToRead(2)) {
        std::this_thread::sleep_for(std::chrono::milliseconds((long)100));
    }

    _shared_memory.read_cursor = 1;

    auto user_id_data = _shared_memory.readNext(sizeof(int64_t));
    int64_t received_user_id = *reinterpret_cast<const int64_t*>(user_id_data.data());
    if (received_user_id != user_id) {
        throw ServerException("User ID mismatch");
    }

    auto filename_size_data = _shared_memory.readNext(sizeof(ushort));
    ushort result_filename_size = *reinterpret_cast<const ushort*>(filename_size_data.data());

    auto filename_data = _shared_memory.readNext(result_filename_size);
    std::string result_filename(filename_data.begin(), filename_data.end());

    auto file_size_data = _shared_memory.readNext(sizeof(uint32_t));
    uint32_t result_file_size = *reinterpret_cast<const uint32_t*>(file_size_data.data());

    auto result_file_data = _shared_memory.readNext(result_file_size);

    DataBuffer response{FILE_RESPONSE};
    response.insert(response.end(), result_filename.begin(), result_filename.end());
    response.insert(response.end(), result_file_data.begin(), result_file_data.end());

    clientConnection.socket->send(response);

    _shared_memory.clear();
    clientConnection._current_state = MAIN_MENU;
}

void Server::HandleGameMove(socket_t sock, int move, ClientConnection& clientConnection) {
    try {
        _message_queue.send(sock, move, CLIENT_MOVE);

        auto response = _message_queue.receive(sock, SERVER_MOVE);

        DataBuffer response_buf;

        if (response.arg < 0) {
            if (response.arg == -1) {
                response_buf.push_back(GAME_WIN);
            } else {
                response_buf.push_back(GAME_LOSE);
            }
            clientConnection._current_state = MAIN_MENU;
        } else {
            response_buf.push_back(GAME_UPDATE);
            std::string msg = "Sticks left: " + std::to_string(response.arg);
            response_buf.insert(response_buf.end(), msg.begin(), msg.end());
        }

        clientConnection.socket->send(response_buf);
    }
    catch (const IPCException& e) {
        _logger->LogError(std::format("IPC error during game move: {}", e.what()));

        DataBuffer error_response{ERROR_RESPONSE};
        std::string err_msg = "Game error: " + std::string(e.what());
        error_response.insert(error_response.end(), err_msg.begin(), err_msg.end());

        clientConnection.socket->send(error_response);

        clientConnection._current_state = MAIN_MENU;
    }
}

void FileToCompilationData(const std::filesystem::path& path,
                           ushort& filename_size,
                           std::string& filename,
                           uint32_t& size,
                           std::vector<char>& data) {

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File not found: " + path.string());
    }

    filename = path.filename().string();
    filename_size = filename.size();

    size = std::filesystem::file_size(path);
    data.resize(size);

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    file.read(data.data(), size);
}