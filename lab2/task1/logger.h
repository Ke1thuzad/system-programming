#ifndef SYSTEM_PROGRAMMING_LOGGER_H
#define SYSTEM_PROGRAMMING_LOGGER_H

#include "../task2/thread_safe_queue.h"

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

enum LogLevels {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
    inline static std::mutex log_file_op_mutex{};
    std::mutex instance_mutex{};

    LogLevels levelBorder = WARNING;
    std::unique_ptr<std::ofstream> file_writer_ptr;
    std::ostream* writer = nullptr;
    ThreadSafeQueue<std::string>* target_queue = nullptr;

    std::string logDirectory = ".";
    std::string filePrefix = "log";

    Logger(LogLevels level, std::string dir, std::string prefix)
            : levelBorder(level), logDirectory(std::move(dir)), filePrefix(std::move(prefix))
    {
        OpenLogFile();
    }

    Logger(LogLevels level, std::ostream* stream)
            : levelBorder(level), writer(stream)
    {
        if (!writer || !writer->good()) {
            throw std::runtime_error("ostream is invalid.");
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
        LogLevels level = WARNING;
        std::ostream* ext_ostream_ptr = nullptr;
        ThreadSafeQueue<std::string>* queue_ptr = nullptr;
        std::string directory = "logs";
        std::string prefix = "log";

    public:
        Builder& SetLogLevel(LogLevels lvl) { level = lvl; return *this; }
        Builder& SetDirectory(const std::string& dir) {
            if (ext_ostream_ptr || queue_ptr)
                throw std::runtime_error("Cannot set directory when using stream or queue.");

            directory = dir;
            return *this;
        }
        Builder& SetPrefix(const std::string& pfx) {
            if (ext_ostream_ptr || queue_ptr)
                throw std::runtime_error("Cannot set prefix when using stream or queue.");

            prefix = pfx;
            return *this;
        }
        Builder& SetStream(std::ostream* stream) {
            if (queue_ptr)
                throw std::runtime_error("Cannot set both ostream and queue");
            if (!stream || !stream->good())
                throw std::runtime_error("ostream for logger is not valid.");

            ext_ostream_ptr = stream;
            return *this;
        }
        Builder& SetQueue(ThreadSafeQueue<std::string>* queue) {
            if (ext_ostream_ptr)
                throw std::runtime_error("Cannot set both ostream and queue");

            if (!queue)
                throw std::runtime_error("queue pointer for logger is null.");

            queue_ptr = queue;
            return *this;
        }

        std::unique_ptr<Logger> Build() {
            if (queue_ptr)
                return std::unique_ptr<Logger>(new Logger(level, queue_ptr));

            if (ext_ostream_ptr)
                return std::unique_ptr<Logger>(new Logger(level, ext_ostream_ptr));

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