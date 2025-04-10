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
    std::ofstream writer {};

    std::string GenerateLogName(const std::string& prefix = "app");
    static const char* logLevelToString(LogLevels lvl);
public:
    Logger() : levelBorder(WARNING) { OpenLogFile(); }
    explicit Logger(LogLevels level) : levelBorder(level) { OpenLogFile(); }
    explicit Logger(const std::string &prefix) : levelBorder(WARNING) { OpenLogFile(prefix); }
    Logger(LogLevels level, const std::string &path) : levelBorder(level) { OpenLogFile(path); }

    ~Logger() { writer.close(); }

    void OpenLogFile();
    void OpenLogFile(const std::string &path);

    void Log(LogLevels msgLevel, const std::string &message);
    void LogDebug(const std::string &message) { Log(DEBUG, message); };
    void LogInfo(const std::string &message) { Log(INFO, message); };
    void LogWarning(const std::string &message) { Log(WARNING, message); };
    void LogError(const std::string &message) { Log(ERROR, message); };
    void LogCritical(const std::string &message) { Log(CRITICAL, message); };
};

#endif //SYSTEM_PROGRAMMING_LOGGER_H
