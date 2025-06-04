#include "Client.hpp"

Client::Client(const std::string& host, uint16_t port, const std::string& download_dir)
        : _client(std::make_unique<TcpClient>()),
          _download_dir(download_dir) {

    if (!std::filesystem::exists(_download_dir)) {
        std::filesystem::create_directories(_download_dir);
    }

    Connect(host, port);
}

void Client::Connect(const std::string& host, uint16_t port) {
    try {
        _client->connect(host, port);
        _is_connected = true;
        std::cout << "Connected to server\n";
    } catch (const SocketException& e) {
        std::cerr << "Connection failed: " << e.what() << "\n";
        _is_connected = false;
    }
}

void Client::Disconnect() {
    if (_is_connected) {
        DataBuffer disconnect_cmd{'3'};
        _client->send(disconnect_cmd);

        _client->disconnect();
        _is_connected = false;
        std::cout << "Disconnected from server\n";
    }
}

void Client::SendCommand(uint8_t command) {
    DataBuffer buffer{command};
    _client->send(buffer);
}

void Client::SendFile(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path) || !std::filesystem::is_regular_file(file_path)) {
        throw std::runtime_error("File not found");
    }

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    size_t file_size = file.tellg();
    file.seekg(0);

    DataBuffer file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    file.close();

    DataBuffer request{COMPILE_COMMAND};
    std::string filename = file_path.filename().string();
    request.insert(request.end(), filename.begin(), filename.end());
    request.push_back('\0');
    request.insert(request.end(), file_data.begin(), file_data.end());

    _client->send(request);
    std::cout << "File sent for compilation\n";
}

void Client::HandleServerResponse() {
    try {
        DataBuffer response = _client->receive();
        if (response.empty()) {
            std::cerr << "Empty response from server\n";
            return;
        }

        uint8_t response_type = response[0];

        switch (response_type) {
            case FILE_RESPONSE: {
                auto null_pos = std::find(response.begin() + 1, response.end(), '\0');
                if (null_pos == response.end()) {
                    throw std::runtime_error("Invalid file response");
                }

                std::string filename(response.begin() + 1, null_pos);
                std::vector<uint8_t> file_data(null_pos + 1, response.end());

                std::filesystem::path save_path = _download_dir / filename;
                std::ofstream out_file(save_path, std::ios::binary);
                out_file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
                out_file.close();

                std::cout << "File saved to: " << save_path << "\n";
                break;
            }
            case ERROR_RESPONSE: {
                std::string error_msg(response.begin() + 1, response.end());
                std::cerr << "Error: " << error_msg << "\n";
                break;
            }
            case GAME_START: {
                std::string msg(response.begin() + 1, response.end());
                std::cout << "Game: " << msg << "\n";
                _in_game = true;
                break;
            }
            case GAME_UPDATE: {
                std::string msg(response.begin() + 1, response.end());
                std::cout << "Game: " << msg << "\n";
                break;
            }
            case GAME_WIN: {
                std::cout << "You win! Game over.\n";
                _in_game = false;
                break;
            }
            case GAME_LOSE: {
                std::cout << "You lose! Game over.\n";
                _in_game = false;
                break;
            }
            default:
                std::cerr << "Unknown response type: " << static_cast<int>(response_type) << "\n";
        }
    }
    catch (const SocketDisconnectedException&) {
        std::cerr << "Connection closed by server\n";
        _is_connected = false;
    }
    catch (const SocketException& e) {
        std::cerr << "Network error: " << e.what() << "\n";
        _is_connected = false;
    }
}

void Client::StartGame() {
    SendCommand(GAME_COMMAND);
    _in_game = true;
}

void Client::MakeMove(int move) {
    if (move < 1 || move > 3) {
        throw std::invalid_argument("Invalid move");
    }

    DataBuffer move_cmd{static_cast<uint8_t>('0' + move)};
    _client->send(move_cmd);
}

void Client::ShowMainMenu() {
    std::cout << "\n===== Main Menu =====\n";
    std::cout << "1. Compile file\n";
    std::cout << "2. Play Sticks game\n";
    std::cout << "3. Disconnect\n";
    std::cout << "=====================\n";
    std::cout << "Select option: ";
}