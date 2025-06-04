#include "Server.hpp"

void FileToCompilationData(const std::filesystem::path &path, ushort &filename_size, std::string &filename, uint32_t &size,
                      char **data) {
    std::ifstream reader(path, std::ios_base::binary | std::ios_base::in | std::ios_base::ate);
    if (!reader.good()) {
        throw std::runtime_error("File was not opened");
    }

    filename_size = path.filename().string().length() + 1;
    filename = path.filename().string();

    size = static_cast<uint32_t>(reader.tellg());

    reader.seekg(0, std::ios::beg);

    *data = new char[size];

    reader.read(*data, size);

    reader.close();
}
