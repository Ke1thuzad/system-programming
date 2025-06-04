#pragma once

#include <atomic>
#include <thread>
#include <csignal>
#include <unordered_map>

#include "Exceptions.hpp"
#include "Logger.h"
#include "MessageQueue.hpp"

class SticksGame {
    int min_pick;
    int max_pick;

    bool move;
public:
    int current_amount;

    explicit SticksGame(int stick_count = 15, int min_pick_amount = 1, int max_pick_amount = 3) : min_pick(min_pick_amount),
                                                                                                  max_pick(max_pick_amount),
                                                                                                  move(true),
                                                                                                  current_amount(stick_count) {}
    ~SticksGame() = default;

    void Move(int move_amount);

    void BotMove();

    [[nodiscard]] int CheckWin() const;
};

class GameWorker {
    MessageQueue _messageQueue;
    std::unique_ptr<Logger> _logger;

    std::unordered_map<int64_t, SticksGame> _games;

    std::thread _worker_thread;
    volatile bool _isRunning;

    static void signal_handler(int signum) {
        if (signum == SIGINT || signum == SIGTERM || signum == SIGKILL || signum == SIGTSTP) {
            global_stop_requested.store(true);
        }
    }
public:
    enum messageTypes {
        CLIENT_CONNECT = 3,
        CLIENT_MOVE,
        SERVER_MOVE
    };

    inline static std::atomic<bool> global_stop_requested;

    GameWorker(const char *token_path, int proj_id, LogLevels levelBorder = WARNING);

    ~GameWorker();

    void Run();

    int HandleClientCommand();

    void NewGame(int64_t user_id);

    void MoveCommand(int64_t user_id, int move_amount);

    void FinishGame(std::unordered_map<int64_t, SticksGame>::iterator game, int win);
};