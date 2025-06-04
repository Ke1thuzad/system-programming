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
            continue;
        }

        ReadNextFile();
        if (!_fileQueue.IsEmpty()) {
            std::pair<int64_t, fspath> args;

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
    // [1 byte read flag (server(1)/client(2))] [8 bytes int64_t user_id] [2 bytes ushort filename_size] [--- *filename_size* bytes for filename ---] [4 bytes uint32_t size] [--- *size* bytes of file data ---] --- UP TO 10MB
    try {
        _sharedMemory.read_cursor = 1;

        _sharedMemory.block();

        std::vector<char> user_id_vec = _sharedMemory.read(sizeof(int64_t), _sharedMemory.read_cursor);
        int64_t user_id = *reinterpret_cast<const int64_t*>(user_id_vec.data());

        std::vector<char> filename_size_vec = _sharedMemory.readNext(sizeof(ushort));
        ushort filename_size = *reinterpret_cast<const ushort*>(filename_size_vec.data());

        std::vector<char> filename_vec = _sharedMemory.readNext(filename_size);
        std::string filename(filename_vec.begin(), filename_vec.end());

        std::vector<char> file_size_vec = _sharedMemory.readNext(sizeof(uint32_t));
        uint32_t file_size = *reinterpret_cast<const uint32_t*>(file_size_vec.data());

        std::vector<char> file_data = _sharedMemory.readNext(file_size);

        _sharedMemory.clear();

        _sharedMemory.unblock();

        fspath path("./files/");
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
        path /= filename;

        {
            std::ofstream writer(path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
            if (!writer.good()) {
                throw SubServerException("Writer is bad");
            }

            writer.write(file_data.data(), file_data.size());
        }

        _fileQueue.Push({user_id, path});

    } catch (const std::exception& e) {
        _logger->LogError("Error reading file from shared memory");
        _sharedMemory.clear();
        throw;
    }
}

void CompilerWorker::SendFileBack(int64_t user_id, const fspath &path) {
    try {
        ushort filename_size;
        std::string filename;
        uint32_t size;
        std::vector<char> data;

        FileToCompilationData(path, filename_size, filename, size, data);

        uint8_t flag = 2;
        _sharedMemory.clear();

        _sharedMemory.block();

        _sharedMemory.write(std::span<const char>(reinterpret_cast<const char*>(&flag), 1));
        _sharedMemory.write(std::span<const char>(reinterpret_cast<const char*>(&user_id), sizeof(user_id)));
        _sharedMemory.write(std::span<const char>(reinterpret_cast<const char*>(&filename_size), sizeof(filename_size)));
        _sharedMemory.write(std::span<const char>(filename.data(), filename_size));
        _sharedMemory.write(std::span<const char>(reinterpret_cast<const char*>(&size), sizeof(size)));
        _sharedMemory.write(std::span<const char>(data.data(), data.size()));

        _sharedMemory.unblock();

    } catch (const std::exception& e) {
        _logger->LogError("Error sending file back");
        throw;
    }
}

void CompilerWorker::Compile(int64_t user_id, const CompilerWorker::fspath &path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

    if (std::equal(ext.begin(), ext.end(), ".cpp"))
        CompileCPP(user_id, path);
    else if (std::equal(ext.begin(), ext.end(), ".tex"))
        CompileTex(user_id, path);
    else
        throw SubServerException("Incorrect file format: " + path.extension().string());
    _logger->LogInfo("Successfully compiled file: " + path.string());
}

void CompilerWorker::CompileCPP(int64_t user_id, const CompilerWorker::fspath &path) {
    pid_t pid = fork();
    if (pid == 0) {
        chdir(path.parent_path().c_str());
        execlp("/usr/bin/g++", "g++", path.filename().c_str(), "-o", (path.stem().string() + ".out").c_str(), nullptr);

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
        new_path.replace_extension(".out");

        if (!std::filesystem::exists(new_path))
            throw SubServerException("C++ file was not compiled into executable file");

        SendFileBack(user_id, new_path);
    } else {
        throw SubServerException("Fork failed");
    }
}

void CompilerWorker::CompileTex(int64_t user_id, const CompilerWorker::fspath &path) {
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
                           std::vector<char>& data) {
    std::ifstream reader(path, std::ios_base::binary | std::ios_base::in | std::ios_base::ate);
    if (!reader.good()) {
        throw std::runtime_error("File was not opened");
    }

    filename = path.filename().string();
    filename_size = filename.size() + 1;

    size = static_cast<uint32_t>(reader.tellg());
    data.resize(size);

    reader.seekg(0, std::ios::beg);
    reader.read(data.data(), size);
    reader.close();
}