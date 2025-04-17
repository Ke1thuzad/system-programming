#ifndef SYSTEM_PROGRAMMING_UTILS_H
#define SYSTEM_PROGRAMMING_UTILS_H

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <arpa/inet.h> // For inet_ntop, inet_pton
#include <stdexcept>   // For runtime_error
#include <ctime>

// Function to get current timestamp as string (used internally by Logger, but can be standalone)
inline std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = {};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&now_tm, &now_c);
#else
    localtime_r(&now_c, &now_tm); // POSIX thread-safe version
#endif
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d_%H:%M:%S");
    // Add milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// Function to convert in_addr_t (network byte order) to string
inline std::string ipToString(in_addr_t ip_addr_nbo) {
    char buffer[INET_ADDRSTRLEN];
    // inet_ntop expects network byte order
    if (inet_ntop(AF_INET, &ip_addr_nbo, buffer, INET_ADDRSTRLEN) == nullptr) {
        // Handle error, perhaps throw or return an error string
        return "invalid_ip";
    }
    return std::string(buffer);
}

// Function to convert string to in_addr_t (network byte order)
inline bool stringToIp(const std::string& ip_str, in_addr_t& ip_addr_nbo) {
// inet_pton returns 1 on success, 0 if input is not valid, -1 on error
int result = inet_pton(AF_INET, ip_str.c_str(), &ip_addr_nbo);
return result == 1;
}

// Function to convert in_port_t (network byte order) to integer
inline uint16_t portToUint16(in_port_t port_nbo) {
    return ntohs(port_nbo); // Convert from network to host byte order
}

// Function to convert integer to in_port_t (network byte order)
inline in_port_t uint16ToPort(uint16_t port_hbo) {
    return htons(port_hbo); // Convert from host to network byte order
}

#endif // SYSTEM_PROGRAMMING_UTILS_H