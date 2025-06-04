#include "Server.hpp"
#include <csignal>

std::atomic<bool> running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running.store(false);
    }
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        Server server("/tmp", 1234, 38902, DEBUG);
        server.Start();

        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds((long) 1));
        }

        server.Stop();
    } catch (const std::exception& e) {
        std::cerr << "Server fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}