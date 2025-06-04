#pragma once

#include <filesystem>
#include <fstream>
#include <atomic>
#include <iostream>
#include <format>
#include <cctype>

#include "TCPClient.hpp"

#define COMPILE_COMMAND ('0')
#define GAME_COMMAND ('1')
#define DISCONNECT_COMMAND ('3')
#define FILE_RESPONSE ('F')
#define ERROR_RESPONSE ('E')
#define GAME_UPDATE ('S')
#define GAME_WIN ('W')
#define GAME_LOSE ('L')
#define GAME_START ('G')

class Client {
    std::unique_ptr<TcpClient> _client;
    std::atomic<bool> _is_connected{false};
    std::filesystem::path _download_dir;
    std::atomic<bool> _in_game{false};

public:
    Client(const std::string& host, uint16_t port, const std::string& download_dir = "./downloads");
    void Connect(const std::string& host, uint16_t port);
    void Disconnect();
    [[nodiscard]] bool IsConnected() const { return _is_connected; }

    void SendCommand(uint8_t command);
    void SendFile(const std::filesystem::path& file_path);
    void HandleServerResponse();

    void StartGame();
    void MakeMove(int move);

    void ShowMainMenu();

    bool InGame() const { return _in_game.load(); }
};