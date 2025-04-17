#include "generator.h"
#include "utils.h"
#include "../task1/logger.h"

#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>

in_addr_t generateIP(std::default_random_engine & gen) {
    return htonl(0xC0A86400 | (gen() % 0x0000FF));
}

in_port_t generatePort(std::default_random_engine & gen) {
    return htons(gen() % 0xFFFF);
}

tcp_traffic_session generateSessionTraffic(std::default_random_engine & gen) {
    in_addr_t src_ip = generateIP(gen);
    tcp_traffic_session session(src_ip);
    size_t num_data_pkgs = 1 + (gen() % 10);
    session.pkgs.reserve(num_data_pkgs);

    in_port_t common_src_port = generatePort(gen);
    in_addr_t common_dst_ip = generateIP(gen);
    while (src_ip == common_dst_ip) {
        common_dst_ip = generateIP(gen);
    }
    in_port_t common_dst_port = generatePort(gen);

    for (size_t i = 0; i < num_data_pkgs; ++i) {
        size_t data_size = gen() % 1401;
        session.pkgs.emplace_back(common_src_port, common_dst_ip, common_dst_port, data_size);
    }
    return session;
}

void sessionToLogs(const tcp_traffic_session& session, Logger* logger, std::default_random_engine & gen) {
    if (!logger || session.pkgs.empty()) {
        return;
    }

    const auto& first_pkg = session.pkgs.front();
    std::string src_ip_str = ipToString(session.src_addr);
    std::string dst_ip_str = ipToString(first_pkg.dst_addr);
    uint16_t src_port_int = portToUint16(first_pkg.src_port);
    uint16_t dst_port_int = portToUint16(first_pkg.dst_port);
    std::ostringstream oss;

    oss.str("");
    oss.clear();

    oss << "Event: Connect SrcIP: " << src_ip_str << " SrcPort: " << src_port_int
        << " DstIP: " << dst_ip_str << " DstPort: " << dst_port_int;

    logger->LogInfo(oss.str());

    for (const auto& pkg : session.pkgs) {
        oss.str(""); oss.clear();
        oss << "Event: SendData SrcIP: " << src_ip_str
            << " DstIP: " << dst_ip_str << " SrcPort: " << portToUint16(pkg.src_port)
            << " DstPort: " << portToUint16(pkg.dst_port) << " Size: " << pkg.sz;
        logger->LogInfo(oss.str());
    }

    oss.str("");
    oss.clear();

    bool planned_disconnect = (gen() % 10) < 8;
    const char* disconnect_event = planned_disconnect ? "Disconnect" : "Disconnect_Abrupt";
    LogLevels level = planned_disconnect ? INFO : WARNING;

    oss << "Event: " << disconnect_event << " SrcIP: " << src_ip_str
        << " SrcPort: " << src_port_int << " DstIP: " << dst_ip_str
        << " DstPort: " << dst_port_int;
    logger->Log(level, oss.str());
}

bool checkThreadShutdown(const volatile bool* flag_ptr, pthread_mutex_t* mutex_ptr) {
    if (!flag_ptr || !mutex_ptr)
        return true;

    pthread_mutex_lock(mutex_ptr);
    bool shutdown = *flag_ptr;
    pthread_mutex_unlock(mutex_ptr);
    return shutdown;
}

void* generatorThread(void* data) {
    auto *threadData = static_cast<GeneratorThreadData *>(data);
    if (!threadData || !threadData->queue_ptr || !threadData->shutdown_flag_ptr) {
        fprintf(stderr, "Generator thread FATAL Error: Invalid thread data pointer or internal pointers.\n");
        return nullptr;
    }

    std::random_device rd;
    std::default_random_engine gen(rd() + threadData->thread_id);

    std::unique_ptr<Logger> logger;
    try {
        logger = Logger::Builder()
                .SetLogLevel(threadData->log_level)
                .SetQueue(threadData->queue_ptr)
                .Build();
    } catch (const std::exception &e) {
        fprintf(stderr, "Generator thread %d FATAL Error: Failed to create logger: %s\n", threadData->thread_id,
                e.what());
        return nullptr;
    }

    printf("Generator thread %d started (logging to queue).\n", threadData->thread_id);

    try {
        while (!checkThreadShutdown(threadData->shutdown_flag_ptr, threadData->shutdown_mutex_ptr)) {
            tcp_traffic_session session = generateSessionTraffic(gen);

            sessionToLogs(session, logger.get(), gen);

            std::this_thread::sleep_for(std::chrono::milliseconds(50 + (gen() % 150)));
        }
    } catch (const std::exception &e) {
        fprintf(stderr, "Generator thread %d caught exception: %s\n", threadData->thread_id, e.what());
    }

    printf("Generator thread %d shutting down.\n", threadData->thread_id);

    return nullptr;
}