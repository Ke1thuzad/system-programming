#include "Client.hpp"
#include <csignal>
#include <atomic>

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
        Client client("127.0.0.1", 38902, "./downloads");

        while (running && client.IsConnected()) {
            client.ShowMainMenu();

            std::string choice;
            std::cin >> choice;

            if (choice == "1") {
                std::cout << "Enter file path: ";
                std::string file_path;
                std::cin >> file_path;

                client.SendFile(file_path);
                client.HandleServerResponse();
            }
            else if (choice == "2") {
                client.StartGame();

                while (client.IsConnected() && client.InGame()) {
                    client.HandleServerResponse();

                    if (!client.InGame()) break;

                    std::cout << "Your move (1-3, 0 to quit): ";
                    std::string input;
                    std::cin >> input;

                    if (input == "0") {
                        client.SendCommand(DISCONNECT_COMMAND);
                        break;
                    }

                    if (std::isdigit(input[0])) {
                        int move = input[0] - '0';
                        if (move >= 1 && move <= 3) {
                            client.MakeMove(move);
                        } else {
                            std::cout << "Invalid move. Please enter 1, 2 or 3.\n";
                        }
                    } else {
                        std::cout << "Invalid input. Please enter a number.\n";
                    }
                }
            }
            else if (choice == "3") {
                client.Disconnect();
                running = false;
            }
            else {
                std::cout << "Invalid choice. Please select 1, 2 or 3.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Client stopped\n";
    return 0;
}