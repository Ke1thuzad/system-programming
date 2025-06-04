#pragma once

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/wait.h>
#include <thread>
#include <csignal>

#include "Exceptions.hpp"
#include "SharedMemory.hpp"
#include "ThreadSafeQueue.h"
#include "Logger.h"

#define MAX_FILE_SIZE (10 * (2 << 22))  // 10 MB

#define SIZE_OFFSET (sizeof(uint32_t))

void FileToCompilationData(const std::filesystem::path& path, ushort &filename_size, std::string& filename, uint32_t &size, std::vector<char>& data);

class CompilerWorker {
    using fspath = std::filesystem::path;

    Semaphore _semaphore;
    SharedMemory _sharedMemory;

    ThreadSafeQueue<std::pair<int64_t, fspath>> _fileQueue;

    std::unique_ptr<Logger> _logger;

    std::thread _worker_thread;
    volatile bool _isRunning;

    static void signal_handler(int signum) {
        if (signum == SIGINT || signum == SIGTERM || signum == SIGKILL || signum == SIGTSTP) {
            global_stop_requested.store(true);
        }
    }

public:
    inline static std::atomic<bool> global_stop_requested;

    CompilerWorker(const char *token_path, int proj_id, LogLevels levelBorder = WARNING);

    ~CompilerWorker();

    void Run();

    void ReadNextFile();

    void SendFileBack(int64_t user_id, const fspath &path);

    void Compile(int64_t user_id, const CompilerWorker::fspath &path);

    void CompileCPP(int64_t user_id, const CompilerWorker::fspath &path);

    void CompileTex(int64_t user_id, const CompilerWorker::fspath &path);
};
