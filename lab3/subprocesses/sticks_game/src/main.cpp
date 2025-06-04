#include "GameWorker.hpp"

int main() {
    try {
        GameWorker compiler("/tmp", 1234, DEBUG);

        while (!GameWorker::global_stop_requested.load()) {
            std::this_thread::sleep_for(std::chrono::seconds((long) 1));
        }

        std::cout << "Main: получен сигнал завершения" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }


    return 0;
}