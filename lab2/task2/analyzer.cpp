#include "analyzer.h"
#include "utils.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <cerrno>
#include <stdexcept>


bool parseLogLineSimple(const std::string& log_line, std::map<std::string, std::string>& fields) {
    fields.clear();

    size_t level_end_bracket_pos = log_line.find("] ");
    if (level_end_bracket_pos == std::string::npos) {
        return false;
    }

    size_t message_start_pos = level_end_bracket_pos + 2;
    if (message_start_pos >= log_line.length()) {
        return false;
    }

    size_t current_pos = message_start_pos;
    while (current_pos < log_line.length()) {
        size_t colon_pos = log_line.find(':', current_pos);
        if (colon_pos == std::string::npos) {
            break;
        }

        size_t key_start_pos = log_line.rfind(' ', colon_pos);
        if (key_start_pos == std::string::npos || key_start_pos < current_pos) {
            key_start_pos = current_pos;
        } else {
            key_start_pos += 1;
        }

        std::string key = log_line.substr(key_start_pos, colon_pos - key_start_pos);
        if (key.empty()) {
            current_pos = colon_pos + 1;
            continue;
        }

        size_t value_start_pos = colon_pos + 1;
        if (value_start_pos < log_line.length() && log_line[value_start_pos] == ' ') {
            value_start_pos++;
        }

        size_t next_key_indicator_pos = std::string::npos;
        size_t search_from = value_start_pos;
        while(search_from < log_line.length()) {
            size_t next_colon = log_line.find(':', search_from);
            if (next_colon == std::string::npos)
                break;

            size_t potential_key_start = log_line.rfind(' ', next_colon);
            if (potential_key_start != std::string::npos && potential_key_start > colon_pos) {
                if (potential_key_start + 1 < next_colon && isupper(log_line[potential_key_start + 1])) {
                    next_key_indicator_pos = potential_key_start;
                    break;
                }
            }
            search_from = next_colon + 1;
        }


        size_t value_end_pos = (next_key_indicator_pos == std::string::npos) ? log_line.length() : next_key_indicator_pos;

        std::string value = log_line.substr(value_start_pos, value_end_pos - value_start_pos);

        fields[key] = value;

        current_pos = value_end_pos;
        if (current_pos < log_line.length() && log_line[current_pos] == ' ') {
            current_pos++;
        }

    }

    if (fields.find("Event") == fields.end() || fields.find("SrcIP") == fields.end()) {
        return false;
    }

    return true;
}

bool updateStatsFromFields(const std::map<std::string, std::string>& fields, IpStats& stats_entry, const std::string& perspective_ip) {
    try {
        const std::string& event_type = fields.at("Event");
        const std::string& src_ip = fields.at("SrcIP");
        std::string dst_ip = fields.count("DstIP") ? fields.at("DstIP") : "";

        uint16_t dst_port = 0;
        if (fields.count("DstPort")) {
            try { dst_port = static_cast<uint16_t>(std::stoul(fields.at("DstPort"))); } catch (...) {}
        }
        size_t size = 0;
        if (fields.count("Size")) {
            try { size = std::stoul(fields.at("Size")); } catch (...) {}
        }

        if (perspective_ip == src_ip) {
            if (event_type == "Connect") {
                stats_entry.connections_initiated++;
                if (!dst_ip.empty())
                    stats_entry.peer_ips.insert(dst_ip);
            }
            else if (event_type == "Disconnect" || event_type == "Disconnect_Abrupt")
                stats_entry.disconnects++;
            else if (event_type == "SendData") {
                stats_entry.send_data_events++;
                stats_entry.total_sent_bytes += size;
                if (!dst_ip.empty())
                    stats_entry.peer_ips.insert(dst_ip);
                if (dst_port > 0) {
                    stats_entry.sent_to_ports[dst_port].sent_bytes += size;
                    stats_entry.sent_to_ports[dst_port].send_packets++;
                }
            } else if (event_type == "ReceiveData") {
                stats_entry.receive_data_events++;
                stats_entry.total_received_bytes += size;
                if (!dst_ip.empty()) stats_entry.peer_ips.insert(dst_ip);
            } else
                return false;
        } else if (perspective_ip == dst_ip) {
            if (event_type == "Connect") {
                stats_entry.connections_accepted++;
                stats_entry.peer_ips.insert(src_ip);
            } else if (event_type == "SendData") {
                stats_entry.receive_data_events++;
                stats_entry.total_received_bytes += size;
                stats_entry.peer_ips.insert(src_ip);
            }
        } else
            return false;

    } catch (const std::exception& e) {
        fprintf(stderr, "Analyzer Error updating stats: %s\n", e.what());
        return false;
    }

    return true;
}

IpStats& findOrCreateLocalStats(std::map<std::string, IpStats>& local_stats_map, const std::string& ip_key) {
    if (ip_key.empty() || ip_key == "invalid_ip") {
        throw std::runtime_error("Attempting to find/create stats for an invalid IP key.");
    }
    auto it = local_stats_map.find(ip_key);
    if (it == local_stats_map.end()) {
        it = local_stats_map.emplace(ip_key, IpStats{}).first;
    }
    return it->second;
}

void* analyzerThread(void* data) {
    auto* threadData = static_cast<AnalyzerThreadData*>(data);
    if (!threadData || !threadData->queue_ptr || !threadData->shutdown_flag_ptr || !threadData->shutdown_mutex_ptr || !threadData->analyzer_stats_ptr) {
        return nullptr;
    }

    printf("Analyzer thread %d started.\n", threadData->thread_id);

    int processed_count = 0;
    std::map<std::string, IpStats>& local_stats_map = threadData->analyzer_stats_ptr->local_stats;

    try {
        std::string log_line;
        while (threadData->queue_ptr->Pop(log_line)) {
            processed_count++;

            std::map<std::string, std::string> fields;
            if (!parseLogLineSimple(log_line, fields)) {
                continue;
            }

            const std::string& src_ip_key = fields.at("SrcIP");
            std::string dst_ip_key = fields.count("DstIP") ? fields.at("DstIP") : "";

            try {
                IpStats& src_stats_entry = findOrCreateLocalStats(local_stats_map, src_ip_key);
                updateStatsFromFields(fields, src_stats_entry, src_ip_key);
            } catch (const std::exception& e) {
                fprintf(stderr, "Analyzer %d Error (SrcIP: %s): %s in line: %s\n", threadData->thread_id, src_ip_key.c_str(), e.what(), log_line.c_str());
            }

            if (!dst_ip_key.empty() && dst_ip_key != src_ip_key) {
                try {
                    IpStats& dst_stats_entry = findOrCreateLocalStats(local_stats_map, dst_ip_key);
                    updateStatsFromFields(fields, dst_stats_entry, dst_ip_key);
                } catch (const std::exception& e) {
                    fprintf(stderr, "Analyzer %d Error (DstIP: %s): %s in line: %s\n", threadData->thread_id, dst_ip_key.c_str(), e.what(), log_line.c_str());
                }
            }
        }

    } catch (const std::exception& e) {
        fprintf(stderr, "Analyzer thread %d FATAL Error: Unhandled exception in main loop: %s\n", threadData->thread_id, e.what());
    }

    printf("Analyzer thread %d shutting down. Processed %d logs.\n", threadData->thread_id, processed_count);

    return nullptr;
}