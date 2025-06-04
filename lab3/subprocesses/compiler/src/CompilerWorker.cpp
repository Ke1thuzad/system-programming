#include "CompilerWorker.hpp"

CompilerWorker::CompilerWorker(const char *token_path, int proj_id, LogLevels levelBorder) : _semaphore(token_path,
                                                                                                        proj_id, 1),
                                                                                             _sharedMemory(token_path,
                                                                                                           proj_id,
                                                                                                           MAX_FILE_SIZE),
                                                                                             _fileQueue(),
                                                                                             _isRunning(false) {
    _logger = Logger::Builder().SetLogLevel(levelBorder).SetPrefix("subserver_compiler").Build();

    struct sigaction sa{};
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = signal_handler;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGKILL, &sa, nullptr);
    sigaction(SIGTSTP, &sa, nullptr);

    _isRunning = true;
    _worker_thread = std::thread(&CompilerWorker::Run, this);
}

CompilerWorker::~CompilerWorker() {
    _isRunning = false;

    _fileQueue.Push({-1, fspath()});

    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }

    _logger->LogInfo("Compiler SubServer stopped working");

}

void CompilerWorker::Run() {
    _logger->LogInfo("Compiler SubServer started working");
    while (_isRunning && !global_stop_requested.load()) {
        if (!_sharedMemory.readyToRead(1)) {
//                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        ReadNextFile();
        if (!_fileQueue.IsEmpty()) {
            std::pair<uint64_t, fspath> args;

            _fileQueue.Pop(args);

            fspath file = args.second;

            if (file.empty())
                break;

            try {
                Compile(args.first, file);
            } catch (const SubServerException &exception) {
                _logger->LogWarning(exception.what());
            } catch (const std::exception &exception) {
                _logger->LogError(exception.what());
            }
        }
    }
}

void CompilerWorker::ReadNextFile() {
    // FORMAT:
    // [1 byte read flag (server(1)/client(2))] [8 bytes uint64_t user_id] [2 bytes ushort filename_size] [--- *filename_size* bytes for filename ---] [4 bytes uint32_t size] [--- *size* bytes of file data ---] --- UP TO 10MB

    _sharedMemory.read_cursor = 1;

    char *temp_data_reader = _sharedMemory.readNext(sizeof(uint64_t));

    uint64_t user_id = *((uint64_t *)temp_data_reader);

    delete[] temp_data_reader;

    temp_data_reader = _sharedMemory.readNext(sizeof(ushort));

    ushort filename_size = *((ushort *) temp_data_reader);

    delete[] temp_data_reader;

    char *filename = (char *) _sharedMemory.readNext(filename_size);

    temp_data_reader = _sharedMemory.readNext(SIZE_OFFSET);

    uint32_t size = *((uint32_t *) temp_data_reader);

    delete[] temp_data_reader;

    char *data = _sharedMemory.readNext(size);

    _sharedMemory.clear();


    _semaphore.wait_get(0);

    fspath path("./files/");

    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);

    path /= filename;

    delete[] filename;

    std::ofstream writer(path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);

    if (!writer.good()) {
        throw SubServerException("Writer is bad");
    }

    writer.write(data, size);

    writer.close();

    delete[] data;

    _semaphore.release(0);


    _fileQueue.Push({user_id, path});
}

void CompilerWorker::SendFileBack(uint64_t user_id, const CompilerWorker::fspath &path) {
    ushort filename_size;
    std::string filename;
    uint32_t size;
    char *data;

    FileToCompilationData(path, filename_size, filename, size, &data);

    int flag = 2;

    _sharedMemory.write((void *) &flag, 1);
    _sharedMemory.write((void *) &user_id, sizeof(user_id));
    _sharedMemory.write((void *) &filename_size, sizeof(ushort));
    _sharedMemory.write((void *) filename.c_str(), filename_size);
    _sharedMemory.write((void *) &size, sizeof(uint32_t));
    _sharedMemory.write((void *) data, size);

    delete[] data;
}

void CompilerWorker::Compile(uint64_t user_id, const CompilerWorker::fspath &path) {
    if (path.extension() == ".cpp")
        CompileCPP(user_id, path);
    else if (path.extension() == ".tex")
        CompileTex(user_id, path);
    else
        throw SubServerException("Incorrect file format: " + path.extension().string());
    _logger->LogInfo("Successfully compiled file: " + path.string());
}

void CompilerWorker::CompileCPP(uint64_t user_id, const CompilerWorker::fspath &path) {
    pid_t pid = fork();
    if (pid == 0) {
        chdir(path.parent_path().c_str());
        execlp("/usr/bin/g++", "g++", path.filename().c_str(), "-o", (path.stem().string() + ".o").c_str(), nullptr);

        throw SubServerException("C++ file was not compiled");
    } else if (pid > 0) {
        int status;
        if (waitpid(pid, &status, 0) == -1)
            throw SubServerException("Waitpid error");

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0)
                throw SubServerException("Compilation exited with code: " + std::to_string(exit_status));
        } else
            throw SubServerException("Abnormal child exit");

        fspath new_path = path;
        new_path.replace_extension(".o");

        if (!std::filesystem::exists(new_path))
            throw SubServerException("C++ file was not compiled into executable file");

        SendFileBack(user_id, new_path);
    } else {
        throw SubServerException("Fork failed");
    }
}

void CompilerWorker::CompileTex(uint64_t user_id, const CompilerWorker::fspath &path) {
    pid_t pid = fork();
    if (pid == 0) {
        chdir(path.parent_path().c_str());
        execlp("/usr/bin/pdflatex", "pdflatex", path.filename().c_str(), nullptr);

        throw SubServerException("LaTeX file was not compiled");
    } else if (pid > 0) {
        int status;
        if (waitpid(pid, &status, 0) == -1)
            throw SubServerException("Waitpid error");

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0)
                throw SubServerException("Compilation exited with code: " + std::to_string(exit_status));
        } else
            throw SubServerException("Abnormal child exit");

        fspath new_path = path;
        new_path.replace_extension(".pdf");

        if (!std::filesystem::exists(new_path))
            throw SubServerException("LaTeX file was not compiled into pdf file");

        SendFileBack(user_id, new_path);
    } else {
        throw SubServerException("Fork failed");
    }
}

void FileToCompilationData(const std::filesystem::path &path, ushort &filename_size, std::string &filename, uint32_t &size,
                      char **data) {
    std::ifstream reader(path, std::ios_base::binary | std::ios_base::in | std::ios_base::ate);
    if (!reader.good()) {
        throw std::runtime_error("File was not opened");
    }

    filename_size = path.filename().string().length() + 1;
    filename = path.filename().string();

    size = static_cast<uint32_t>(reader.tellg());

    reader.seekg(0, std::ios::beg);

    *data = new char[size];

    reader.read(*data, size);

    reader.close();
}
