#include <unistd.h>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "Exceptions.hpp"
#include "Semaphore.hpp"
#include "SharedMemory.hpp"
#include "ThreadSafeQueue.h"
#include "Logger.h"

#define MAX_FILE_SIZE (10 * (2 << 22))  // 10 MB

#define SIZE_OFFSET 4

class CompilerWorker {
    using fspath = std::filesystem::path;

    Semaphore _semaphore;
    SharedMemory _sharedMemory;

    ThreadSafeQueue<fspath> _fileQueue;

    LogLevels level;
    std::unique_ptr<Logger> _logger;

    bool isRunning = false;
public:
    CompilerWorker(LogLevels levelBorder = WARNING) : _semaphore("/sys/", 1, 1),
                                                      _sharedMemory("/sys/", 2, MAX_FILE_SIZE), _fileQueue(),
                                                      level(levelBorder) {
        _logger = Logger::Builder().SetLogLevel(level).SetPrefix("subserver_compiler").Build();

        isRunning = true;
        Run();
    }

    void Run() {
        while (isRunning) {
            if (_sharedMemory.isEmpty()) continue;

            ReadNextFile();
            if (!_fileQueue.IsEmpty()) {
                fspath file;
                _fileQueue.Pop(file);

                try {
                    Compile(file);
                } catch (const SubServerException &exception) {
                    _logger->LogWarning(exception.what());
                } catch (const std::exception &exception) {
                    _logger->LogError(exception.what());
                }
            }
        }
    }

    void ReadNextFile() {
        // FORMAT:
        // [2 bytes ushort filename_size] [--- *filename_size* bytes for filename ---] [4 bytes uint32_t size] [--- *size* bytes of file data ---] --- UP TO 10MB

        ushort filename_size = *((ushort *) _sharedMemory.read(SIZE_OFFSET));
        char *filename = (char *) _sharedMemory.read(filename_size, SIZE_OFFSET);

        uint32_t size = *((uint32_t *) _sharedMemory.read(SIZE_OFFSET, SIZE_OFFSET + filename_size));
        void *data = _sharedMemory.read(size, 2 * SIZE_OFFSET + filename_size);

        _sharedMemory.clear();


        _semaphore.wait_get(1);

        fspath path("files/");

        path /= filename;

        std::ofstream writer(path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);

        writer.write((char *) data, size);

        writer.close();

        _semaphore.release(1);


        _fileQueue.Push(path);
    }

    static void Compile(const fspath &path) {
        if (path.extension() == "cpp")
            CompileCPP(path);
        else if (path.extension() == ".tex")
            CompileTex(path);
        else
            throw SubServerException("Incorrect file format");
    }

    static void CompileCPP(const fspath &path) {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("/usr/bin/g++", "g++", path.c_str(), nullptr);

            throw SubServerException("C++ file was not compiled");
        } else {
            // TODO
        }
    }

    static void CompileTex(const fspath &path) {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("/usr/bin/pdflatex", "pdflatex", path.c_str(), nullptr);

            throw SubServerException("LaTeX file was not compiled");
        } else {
            // TODO
        }
    }
};
