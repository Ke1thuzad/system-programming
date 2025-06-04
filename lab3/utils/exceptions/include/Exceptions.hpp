#pragma once
#include <stdexcept>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <cerrno>
#endif

class SocketException : public std::runtime_error {
public:
    explicit SocketException(const std::string& message, int error_code = 0)
            : std::runtime_error(message + ": " + std::to_string(error_code)) {}
};

class SocketInitException : public SocketException {
public:
    explicit SocketInitException(int error_code = 0)
            : SocketException("Socket initialization failed", error_code) {}
};

class SocketBindException : public SocketException {
public:
    explicit SocketBindException(int error_code = 0)
            : SocketException("Socket bind failed", error_code) {}
};

class SocketListenException : public SocketException {
public:
    explicit SocketListenException(int error_code = 0)
            : SocketException("Socket listen failed", error_code) {}
};

class SocketAcceptException : public SocketException {
public:
    explicit SocketAcceptException(int error_code = 0)
            : SocketException("Socket accept failed", error_code) {}
};

class SocketDisconnectedException : public SocketException {
public:
    explicit SocketDisconnectedException(int error_code = 0)
            : SocketException("Connection closed by peer", error_code) {}
};

class SocketConnectionException : public SocketException {
public:
    explicit SocketConnectionException(int error_code = 0)
            : SocketException("Socket connection failed", error_code) {}
};

class SocketOptionException : public SocketException {
public:
    explicit SocketOptionException(int error_code = 0)
            : SocketException("Socket option setting failed", error_code) {}
};

class SocketReadException : public SocketException {
public:
    explicit SocketReadException(int error_code = 0)
            : SocketException("Socket read failed", error_code) {}
};

class SocketWriteException : public SocketException {
public:
    explicit SocketWriteException(int error_code = 0)
            : SocketException("Socket write failed", error_code) {}
};

class IPCException : public std::runtime_error {
public:
    explicit IPCException(const std::string& message)
            : std::runtime_error("[IPC] " + message) {}
};

class ThreadException : public std::runtime_error {
public:
    explicit ThreadException(const std::string& message)
            : std::runtime_error("[Thread] " + message) {}
};

class SubServerException : public std::runtime_error {
public:
    explicit SubServerException(const std::string& message)
            : std::runtime_error("[SubServer] " + message) {}
};

class ServerException : public std::runtime_error {
public:
    explicit ServerException(const std::string& message)
            : std::runtime_error("[Server] " + message) {}
};