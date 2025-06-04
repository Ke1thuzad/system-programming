#pragma once

#include <iostream>
#include <filesystem>

#include "Exceptions.hpp"
#include "MessageQueue.hpp"
#include "Semaphore.hpp"
#include "SharedMemory.hpp"
#include "Logger.h"
#include "TCPServer.hpp"

#define MAX_FILE_SIZE (10 * (2 << 22))  // 10 MB

void FileToCompilationData(const std::filesystem::path &path, ushort &filename_size, std::string &filename, uint32_t &size,
                           char **data);

class Server {
    std::unique_ptr<TcpServer> _server;

    MessageQueue _message_queue;
    SharedMemory _shared_memory;

    std::unique_ptr<Logger> _logger;

    std::vector<std::thread> subserver_threads;

    enum States {
        MAIN_MENU,
        COMPILER,
        GAME
    } _current_state;
public:
    enum messageQueueTypes {
        CLIENT_CONNECT = 3,
        CLIENT_MOVE,
        SERVER_MOVE
    };

    Server(const char *token_path, int proj_id, uint16_t port, LogLevels level = WARNING) : _message_queue(token_path, proj_id),
                                                                                            _shared_memory(token_path, proj_id, MAX_FILE_SIZE),
                                                                                            _current_state(MAIN_MENU) {
        _logger = Logger::Builder().SetPrefix("server").SetLogLevel(level).Build();

        _server = std::make_unique<TcpServer>(port, [this](const DataBuffer& buffer, socket_t sock) {
            HandleClientData(buffer, sock);
        });

        _server->setConnectionHandler([this](socket_t sock) {
            _logger->LogInfo(std::format("Client {} has connected", sock));
        });

        _server->setDisconnectionHandler([this](socket_t sock) {
            _logger->LogInfo(std::format("Client {} has disconnected", sock));

            _message_queue.send(sock, -1, CLIENT_CONNECT);
        });

        _server->start();

        _logger->LogInfo("Server has been started");
    }

    ~Server() {
        _server->stop();

        _logger->LogInfo("Server has been stopped");
    }

    void HandleClientData(const DataBuffer& buffer, socket_t sock) {
        if (buffer.size() > 1) {
            if (_current_state == COMPILER) {
                subserver_threads.emplace_back(&Server::CompilerThread, this, buffer, sock);
            } else {
                throw ServerException("Unknown command");
            }
        }

        unsigned char arg = buffer[0];

        switch (_current_state) {
            case MAIN_MENU:
                MainMenuCommands(sock, arg);
                break;
            case COMPILER:
                throw ServerException("Incorrect file");
                break;
            case GAME:
                subserver_threads.emplace_back(&Server::GameHandler, this, sock, arg);
                break;
            default:
                throw ServerException("Unknown state");
        }
    }

    void MainMenuCommands(socket_t sock, int command) {
        command -= '0';

        switch (command) {
            case 0:
                _current_state = COMPILER;
                break;
            case 1:
                _current_state = GAME;
                _message_queue.send(sock, 1, CLIENT_CONNECT);
                break;
            case 2:
                this->~Server();
                break;
            default:
                throw ServerException("Unknown command");
        }
    }

    void GameHandler(socket_t sock, int command) {
        if (!isdigit(command))
            throw ServerException("Incorrect command");

        _message_queue.send(sock, command, CLIENT_MOVE);

        MessageQueue::Message msg{};

        do {
            msg = _message_queue.receive(SERVER_MOVE);
            if (msg.user_id != sock) {
                _message_queue.send(msg.user_id, msg.arg, msg.mtype);
            } else {
                break;
            }
        } while (true);

        std::string str;

        if ((msg.mtype << 7) != 0) {
            if (msg.arg == 1) {
                str = "You have won a sticks game!";
            } else {
                str = "You have lost a sticks game!";
            }
        } else {
            str = "Server has moved. Current stick amount: " + std::to_string(msg.arg);
        }

        _server->send(sock, {str.begin(), str.end()});
    }

    void CompilerThread(const DataBuffer& buffer, socket_t sock) {
        std::string str(buffer.begin(), buffer.end());
        std::filesystem::path path(str);

        ushort filename_size;
        std::string filename;
        uint32_t size;
        char *data;

        FileToCompilationData(path, filename_size, filename, size, &data);

        int flag = 1;

        while (!_shared_memory.isEmpty()) {}

        _shared_memory.write((void *) &flag, 1);
        _shared_memory.write((void *) &sock, sizeof(sock));
        _shared_memory.write((void *) &filename_size, sizeof(ushort));
        _shared_memory.write((void *) filename.c_str(), filename_size);
        _shared_memory.write((void *) &size, sizeof(uint32_t));
        _shared_memory.write((void *) data, size);

        uint64_t user_id;

        do {
            while (!_shared_memory.readyToRead(2)) {}

            char* temp_data = _shared_memory.read(sizeof(sock), 1);

            user_id = *((uint64_t *) temp_data);

            delete[] temp_data;

            std::this_thread::sleep_for(std::chrono::seconds((long) 1));
        } while (user_id != sock);

        _shared_memory.read_cursor = 1 + sizeof(sock);
        char *temp_data_reader = _shared_memory.readNext(sizeof(ushort));
        ushort new_filename_size = *((ushort *) temp_data_reader);
        delete[] temp_data_reader;
        char *new_filename = (char *) _shared_memory.readNext(new_filename_size);
        temp_data_reader = _shared_memory.readNext(sizeof(uint32_t));
        uint32_t new_size = *((uint32_t *) temp_data_reader);
        delete[] temp_data_reader;
        char *new_data = _shared_memory.readNext(new_size);

        _shared_memory.clear();

        std::filesystem::path new_path("./files/");

        if (!std::filesystem::exists(new_path))
            std::filesystem::create_directories(new_path);

        new_path /= new_filename;

        delete[] new_filename;

        std::ofstream writer(new_path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);

        if (!writer.good()) {
            throw ServerException("Writer is bad");
        }

        writer.write(new_data, new_size);

        writer.close();

        delete[] new_data;
    }
};
