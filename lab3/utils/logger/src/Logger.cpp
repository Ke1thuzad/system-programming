#include "Logger.h"

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = {};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&now_tm, &now_c);
#else
    localtime_r(&now_c, &now_tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d_%H:%M:%S");
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::GenerateLogName(const std::string& dir, const std::string& prefix, LogLevels lvl) {
    std::lock_guard<std::mutex> lock(log_file_op_mutex);

    try {
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Failed to create log directory '" + dir + "': " + e.what());
    }

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = {};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::filesystem::path base_path = std::filesystem::path(dir);
    std::ostringstream filename_base_oss;
    filename_base_oss << prefix << "_" << logLevelToString(lvl) << "_" << std::put_time(&tm, "%Y%m%d_%H%M%S");

    std::string filename_base = filename_base_oss.str();
    std::string extension = ".log";
    std::filesystem::path file_path = base_path / (filename_base + extension);

    int count = 1;
    while (std::filesystem::exists(file_path)) {
        std::ostringstream numbered_filename_oss;
        numbered_filename_oss << filename_base << "_" << count++ << extension;
        file_path = base_path / numbered_filename_oss.str();
        if (count > 1000) {
            throw std::runtime_error("Failed to generate unique log filename in directory: " + dir);
        }
    }
    return file_path.string();
}

void Logger::OpenLogFile() {
    std::string path = GenerateLogName(logDirectory, filePrefix, levelBorder);
    try {
        file_writer_ptr = std::make_unique<std::ofstream>(path, std::ios::app);

        if (!file_writer_ptr || !file_writer_ptr->is_open()) {
            throw std::runtime_error("Cannot open log file: " + path);
        }
        writer = file_writer_ptr.get();

    } catch (const std::exception& e) {
        file_writer_ptr.reset();
        writer = nullptr;
        throw std::runtime_error("Failed to initialize log file '" + path + "': " + e.what());
    }
}


const char* Logger::logLevelToString(LogLevels lvl) {
    switch(lvl) {
        case DEBUG:    return "DEBUG";
        case INFO:     return "INFO";
        case WARNING:  return "WARNING";
        case ERROR:    return "ERROR";
        case CRITICAL: return "CRITICAL";
        default:       return "UNKNOWN";
    }
}

void Logger::Log(LogLevels msgLevel, const std::string& message) {
    if (msgLevel < levelBorder) {
        return;
    }

    std::ostringstream logMessageStream;
    logMessageStream << '[' << getTimestamp() << "] "
                     << '[' << logLevelToString(msgLevel) << "] "
                     << message;

    std::string formatted_message = logMessageStream.str();

    if (target_queue) {
        try {
            target_queue->Push(formatted_message);
        } catch (const std::exception& e) {
            fprintf(stderr, "Logger CRITICAL Error: Failed to push log message to queue: %s\n", e.what());
        }
        return;
    }

    if (writer && writer->good()) {
        std::lock_guard<std::mutex> lock(instance_mutex);
        try {
            *writer << formatted_message << '\n';
            writer->flush();
        } catch (const std::ios_base::failure& e) {
            fprintf(stderr, "Logger Error: Failed to write to log stream: %s\n", e.what());
        }
    }

    else if (!writer || !writer->good()) {
        std::lock_guard<std::mutex> lock(instance_mutex);
        fprintf(stderr, "Logger Warning: No valid output target. Log dropped: %s\n", formatted_message.c_str());
    }
}