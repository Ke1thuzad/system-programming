#include "GameWorker.hpp"

GameWorker::GameWorker(const char *token_path, int proj_id, LogLevels levelBorder) : _messageQueue(token_path, proj_id) {
    _logger = Logger::Builder().SetLogLevel(levelBorder).SetPrefix("subserver_game").Build();

    struct sigaction sa{};
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = signal_handler;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGKILL, &sa, nullptr);
    sigaction(SIGTSTP, &sa, nullptr);

    _isRunning = true;
    _worker_thread = std::thread(&GameWorker::Run, this);
}

GameWorker::~GameWorker() {
    _isRunning = false;

    _messageQueue.send(0, 0, 1);

    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }

    _logger->LogInfo("Game SubServer stopped working");
}

void GameWorker::Run() {
    _logger->LogInfo("Game SubServer started working");
    while (_isRunning && !global_stop_requested.load()) {
        try {
            if (HandleClientCommand())
                break;
        } catch (const SubServerException& exception) {
            _logger->LogWarning(exception.what());
        } catch (const std::exception& exception) {
            _logger->LogError(exception.what());
        }
    }
}

int GameWorker::HandleClientCommand() {
    auto msg = _messageQueue.receive(-CLIENT_MOVE);

    if (msg.user_id == 0 && msg.mtype == 1) {
        return 1;
    }

    if (msg.mtype == CLIENT_CONNECT) {
        if (msg.arg == 1)
            NewGame(msg.user_id);
        else if (msg.arg == -1)
            _games.erase(msg.user_id);
    } else if (msg.mtype == CLIENT_MOVE) {
        MoveCommand(msg.user_id, msg.arg);
    } else {
        throw SubServerException("Unknown command from user: " + std::to_string(msg.user_id));
    }

    return 0;
}

void GameWorker::NewGame(unsigned long long int user_id) {
    _games.emplace(user_id, SticksGame());
    _logger->LogInfo(std::format("User {} has started a new game", user_id));
}

void GameWorker::MoveCommand(unsigned long long int user_id, int move_amount) {
    auto game = _games.find(user_id);
    if (game == _games.end()) {
        throw SubServerException("Player has no active game");
    }

    game->second.Move(move_amount);

    int win = game->second.CheckWin();
    if (win)
        FinishGame(game, win);

    game->second.BotMove();
    win = game->second.CheckWin();
    if (win)
        FinishGame(game, win);

    _messageQueue.send(user_id, game->second.current_amount, SERVER_MOVE);
}

void GameWorker::FinishGame(auto game, int win) {
    if (win == 1) {
        _logger->LogInfo(std::format("User {} has won a game", game->first));

        _messageQueue.send(game->first, 1, SERVER_MOVE | (1 << 7));
    } else {
        _logger->LogInfo(std::format("User {} has lost a game", game->first));

        _messageQueue.send(game->first, -1, SERVER_MOVE | (1 << 7));
    }

    _games.erase(game);
}

void SticksGame::Move(int move_amount) {
    if (current_amount - move_amount < 0 || move_amount > max_pick || move_amount < min_pick)
        throw SubServerException("Invalid move");

    current_amount -= move_amount;

    move = !move;
}

int SticksGame::CheckWin() const {
    if (current_amount <= 0) {
        return (!move) * 2 - 1;
    }

    return 0;
}

void SticksGame::BotMove() {
    int move_amount = min_pick + rand() % (max_pick - min_pick);

    Move(move_amount);
}
