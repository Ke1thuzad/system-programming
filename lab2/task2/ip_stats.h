#ifndef SYSTEM_PROGRAMMING_IP_STATS_H
#define SYSTEM_PROGRAMMING_IP_STATS_H

#include <cstdint>
#include <string>
#include <map>
#include <set>
#include <sstream>
#include <pthread.h>
#include <arpa/inet.h>
#include "utils.h"

#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <cerrno>

inline void initialize_mutex_local(pthread_mutex_t* mutex) {
    int ret = pthread_mutex_init(mutex, nullptr);
    if (ret != 0) {
        throw std::runtime_error("Failed to initialize stats mutex: " + std::string(strerror(ret)));
    }
}

inline void destroy_mutex_local(pthread_mutex_t* mutex) {
    int ret = pthread_mutex_destroy(mutex);
    if (ret != 0 && ret != EBUSY) {
        fprintf(stderr, "Warning: Failed to destroy stats mutex: %s\n", strerror(ret));
    }
}


struct DestPortStats {
    uint64_t sent_bytes = 0;
    uint64_t received_bytes = 0;
    uint32_t send_packets = 0;
    uint32_t receive_packets = 0;

    DestPortStats& operator+=(const DestPortStats& other) {
        sent_bytes += other.sent_bytes;
        received_bytes += other.received_bytes;
        send_packets += other.send_packets;
        receive_packets += other.receive_packets;
        return *this;
    }
};

struct IpStats {
    uint64_t total_sent_bytes = 0;
    uint64_t total_received_bytes = 0;
    uint32_t connections_initiated = 0;
    uint32_t connections_accepted = 0;
    uint32_t disconnects = 0;
    uint32_t send_data_events = 0;
    uint32_t receive_data_events = 0;

    std::set<std::string> peer_ips;
    std::map<uint16_t, DestPortStats> sent_to_ports;

    IpStats& operator+=(const IpStats& other) {
        total_sent_bytes += other.total_sent_bytes;
        total_received_bytes += other.total_received_bytes;
        connections_initiated += other.connections_initiated;
        connections_accepted += other.connections_accepted;
        disconnects += other.disconnects;
        send_data_events += other.send_data_events;
        receive_data_events += other.receive_data_events;

        peer_ips.insert(other.peer_ips.begin(), other.peer_ips.end());

        for (const auto& pair : other.sent_to_ports) {
            sent_to_ports[pair.first] += pair.second;
        }
        return *this;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "  Total Sent Bytes: " << total_sent_bytes << "\n"
           << "  Total Received Bytes: " << total_received_bytes << "\n"
           << "  Connections Initiated: " << connections_initiated << "\n"
           << "  Connections Accepted: " << connections_accepted << "\n"
           << "  Disconnects: " << disconnects << "\n"
           << "  SendData Events: " << send_data_events << "\n"
           << "  ReceiveData Events: " << receive_data_events << "\n"
           << "  Peer IPs (" << peer_ips.size() << "): ";
        bool first = true;
        for (const auto& peer : peer_ips) { if (!first) ss << ", "; ss << peer; first = false; }
        ss << "\n";
        if (!sent_to_ports.empty()) {
            ss << "  Sent Data Breakdown by Dest Port:\n";
            for(const auto& pair : sent_to_ports) {
                ss << "    Port " << pair.first << ": " << pair.second.sent_bytes
                   << " bytes (" << pair.second.send_packets << " packets)\n";
            }
        }
        return ss.str();
    }
};


struct AnalyzerData {
    std::map<std::string, IpStats> local_stats;
    pthread_mutex_t stats_mutex;

    AnalyzerData() {
        initialize_mutex_local(&stats_mutex);
    }

    ~AnalyzerData() {
        destroy_mutex_local(&stats_mutex);
    }
};

#endif // SYSTEM_PROGRAMMING_IP_STATS_H