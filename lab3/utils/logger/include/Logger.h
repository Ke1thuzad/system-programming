#ifndef SYSTEM_PROGRAMMING_LOGGER_H
#define SYSTEM_PROGRAMMING_LOGGER_H

#include "ThreadSafeQueue.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <utility>
#include <string>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <cstdio>

enum LogLevels {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
    inline static std::mutex log_file_op_mutex;
    std::mutex instance_mutex{};

    LogLevels levelBorder = WARNING;
    std::unique_ptr<std::ofstream> file_writer_ptr;
    std::ostream* writer = nullptr;
    ThreadSafeQueue<std::string>* target_queue = nullptr;

    std::string logDirectory = ".";
    std::string filePrefix = "app";

    Logger(LogLevels level, std::string dir, std::string prefix)
            : levelBorder(level), logDirectory(std::move(dir)), filePrefix(std::move(prefix))
    {
        OpenLogFile();
    }

    Logger(LogLevels level, std::ofstream* stream)
            : levelBorder(level), writer(stream)
    {
        if (!writer || !writer->good()) {
            throw std::runtime_error("ofstream is invalid.");
        }
    }

    Logger(LogLevels level, ThreadSafeQueue<std::string>* queue)
            : levelBorder(level), target_queue(queue)
    {
        if (!target_queue) {
            throw std::runtime_error("target queue pointer is null.");
        }
    }

    void OpenLogFile();
    static const char* logLevelToString(LogLevels lvl);
    static std::string GenerateLogName(const std::string& dir, const std::string& prefix, LogLevels lvl);
    static std::string getTimestamp();

public:

    class Builder {
        LogLevels level = INFO;
        std::ofstream* ext_ofstream_ptr = nullptr;
        ThreadSafeQueue<std::string>* queue_ptr = nullptr;
        std::string directory = "logs";
        std::string prefix = "app";

    public:
        Builder& SetLogLevel(LogLevels lvl) { level = lvl; return *this; }
        Builder& SetDirectory(const std::string& dir) {
            if (ext_ofstream_ptr || queue_ptr) throw std::runtime_error("Cannot set directory when using external stream or queue.");
            directory = dir; return *this;
        }
        Builder& SetPrefix(const std::string& pfx) {
            if (ext_ofstream_ptr || queue_ptr) throw std::runtime_error("Cannot set prefix when using external stream or queue.");
            prefix = pfx; return *this;
        }
        Builder& SetStream(std::ofstream* stream) {
            if (queue_ptr) throw std::runtime_error("Cannot set both ofstream and queue");
            if (!stream || !stream->good()) throw std::runtime_error("ofstream for logger is not valid.");
            ext_ofstream_ptr = stream; return *this;
        }
        Builder& SetQueue(ThreadSafeQueue<std::string>* queue) {
            if (ext_ofstream_ptr) throw std::runtime_error("Cannot set both ofstream and queue");
            if (!queue) throw std::runtime_error("queue pointer for logger is null.");
            queue_ptr = queue; return *this;
        }

        std::unique_ptr<Logger> Build() {
            if (queue_ptr) return std::unique_ptr<Logger>(new Logger(level, queue_ptr));
            if (ext_ofstream_ptr) return std::unique_ptr<Logger>(new Logger(level, ext_ofstream_ptr));
            return std::unique_ptr<Logger>(new Logger(level, directory, prefix));
        }
    };

    ~Logger() {
        if (file_writer_ptr && writer && writer->good()) {
            writer->flush();
        }
    }

    void Log(LogLevels msgLevel, const std::string& message);

    void LogDebug(const std::string& message) { Log(DEBUG, message); }
    void LogInfo(const std::string& message) { Log(INFO, message); }
    void LogWarning(const std::string& message) { Log(WARNING, message); }
    void LogError(const std::string& message) { Log(ERROR, message); }
    void LogCritical(const std::string& message) { Log(CRITICAL, message); }
};

#endif // SYSTEM_PROGRAMMING_LOGGER_H