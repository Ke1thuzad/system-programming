#pragma once

#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <atomic>
#include <vector>
#include <cstring>
#include <string>
#include <format>
#include <map>

#include "Exceptions.hpp"
#include "MessageQueue.hpp"
#include "Semaphore.hpp"
#include "SharedMemory.hpp"
#include "Logger.h"
#include "TCPServer.hpp"

#define MAX_FILE_SIZE (10 * (2 << 22))  // 10 MB

#define COMPILE_COMMAND ('0')
#define GAME_COMMAND ('1')
#define DISCONNECT_COMMAND ('3')
#define FILE_RESPONSE ('F')
#define ERROR_RESPONSE ('E')
#define GAME_UPDATE ('S')
#define GAME_WIN ('W')
#define GAME_LOSE ('L')
#define GAME_START ('G')

void FileToCompilationData(const std::filesystem::path &path,
                           ushort &filename_size,
                           std::string &filename,
                           uint32_t &size,
                           std::vector<char> &data);

class Server {
    std::unique_ptr<TcpServer> _server;
    MessageQueue _message_queue;
    SharedMemory _shared_memory;
    Semaphore _clients_sem;

    std::unique_ptr<Logger> _logger;

    enum States {
        MAIN_MENU,
        COMPILER,
        GAME
    };

    std::atomic<bool> _is_running;

    struct ClientConnection {
        std::shared_ptr<TcpSocket> socket;
        States _current_state;
    };

    std::map<socket_t, ClientConnection> _connected_clients;

    void HandleClientData(const DataBuffer &buffer, socket_t sock);

    void HandleMainMenu(socket_t sock, int command, ClientConnection &clientConnection);

    void HandleCompilerRequest(const DataBuffer &buffer, socket_t sock,
                               ClientConnection &clientConnection);

    void HandleGameStart(socket_t sock, ClientConnection &clientConnection);

    void HandleGameMove(socket_t sock, int move,
                        ClientConnection &clientConnection);

public:
    enum MessageQueueTypes {
        CLIENT_CONNECT = 3,
        CLIENT_MOVE,
        SERVER_MOVE
    };

    Server(const char *token_path, int proj_id, uint16_t port, LogLevels level = WARNING);

    ~Server();

    void Start();

    void Stop();
};