#ifndef SYSTEM_PROGRAMMING_LOGGER_H
#define SYSTEM_PROGRAMMING_LOGGER_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>

enum LogLevels {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
    inline static std::mutex log_mutex;
    LogLevels levelBorder;
    std::ofstream writer;
    std::string logDirectory = "./logs";
    std::string filePrefix = "app";

    Logger(LogLevels level, const std::string& dir, const std::string& prefix)
            : levelBorder(level), logDirectory(dir), filePrefix(prefix)
    {
        OpenLogFile();
    }

    void OpenLogFile();
    static const char* logLevelToString(LogLevels lvl);
    static std::string GenerateLogName(const std::string& dir, const std::string& prefix, LogLevels level);
public:
    class Builder {
        LogLevels level = WARNING;
        std::string directory = "./logs";
        std::string prefix = "app";

    public:
        Builder& SetLogLevel(LogLevels lvl) {
            level = lvl;
            return *this;
        }

        Builder& SetDirectory(const std::string& dir) {
            directory = dir;
            return *this;
        }

        Builder& SetPrefix(const std::string& pfx) {
            prefix = pfx;
            return *this;
        }

        std::unique_ptr<Logger> Build() {
            return std::unique_ptr<Logger>(
                    new Logger(level, directory, prefix)
            );
        }
    };

    ~Logger() { writer.close(); }

    void Log(LogLevels msgLevel, const std::string &message);
    void LogDebug(const std::string &message) { Log(DEBUG, message); };
    void LogInfo(const std::string &message) { Log(INFO, message); };
    void LogWarning(const std::string &message) { Log(WARNING, message); };
    void LogError(const std::string &message) { Log(ERROR, message); };
    void LogCritical(const std::string &message) { Log(CRITICAL, message); };
};

#endif //SYSTEM_PROGRAMMING_LOGGER_H
