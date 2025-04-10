#include "logger.h"

int main() {
    try {
        Logger logger;

        logger.LogError("Something really bad happened!");
        logger.LogDebug("Wtf");
        logger.LogWarning("Help me!");
        logger.LogInfo("Huhh");
        logger.LogCritical("Huhh (but critical!!!)");
    } catch (const char *err_msg) {
        std::cout << err_msg << std::endl;
    }
    return 0;
}

std::string Logger::GenerateLogName(const std::string &prefix) {
    std::lock_guard<std::mutex> lock(log_mutex);

    const std::string log_dir = "./logs";
    if (!std::filesystem::exists(log_dir)) {
        std::filesystem::create_directories(log_dir);
    }

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = {};

#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << "./logs/" << prefix << "_" << logLevelToString(levelBorder) << "_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
    std::string base = oss.str() + ".log";

    int count = 1;
    while (std::filesystem::exists(base)) {
        base = oss.str() + "_" + std::to_string(count++) + ".log";
    }

    return base;
}

void Logger::OpenLogFile() {
    std::string path = GenerateLogName();
    writer.open(path, std::ios::app);

    if (!writer.is_open()) {
        throw std::runtime_error("Cannot open log file: " + path);
    }
}

void Logger::OpenLogFile(const std::string &path) {
    writer.open(path, std::ios::app);

    if (!writer.is_open()) {
        throw std::runtime_error("Cannot open log file: " + path);
    }
}

const char *Logger::logLevelToString(LogLevels lvl) {
    switch(lvl) {
        case DEBUG:   return "DEBUG";
        case INFO:    return "INFO";
        case WARNING: return "WARNING";
        case ERROR:   return "ERROR";
        case CRITICAL:return "CRITICAL";
        default:      return "UNKNOWN";
    }
}

void Logger::Log(LogLevels msgLevel, const std::string &message) {
    if (msgLevel < levelBorder)
        return;

    std::ostringstream logMessage;

    logMessage << '[' << logLevelToString(msgLevel) << "] " << message << '\n';

    std::lock_guard<std::mutex> lock(log_mutex);

    writer << logMessage.str();
}
