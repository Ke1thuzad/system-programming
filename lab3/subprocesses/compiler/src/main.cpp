#include "CompilerWorker.hpp"

int main() {
    try {
//        SharedMemory shm("/sys/", 111, MAX_FILE_SIZE);
//
//        std::filesystem::path path = "test.cpp";
//
//        ushort filename_size;
//        std::string filename;
//        uint32_t size;
//        char *data;
//
//        FileToCompilationData(path, filename_size, filename, size, &data);
//
//        int flag = 1;
//
//        shm.write((void *) &flag, 1);
//        shm.write((void *) &filename_size, sizeof(ushort));
//        shm.write((void *) filename.c_str(), filename_size);
//        shm.write((void *) &size, sizeof(uint32_t));
//        shm.write((void *) data, size);

        CompilerWorker compiler("/sys/", 111, DEBUG);

        while (!CompilerWorker::global_stop_requested.load()) {


            std::this_thread::sleep_for(std::chrono::seconds((long) 1));
        }

        std::cout << "Main: получен сигнал завершения" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }


    return 0;
}
