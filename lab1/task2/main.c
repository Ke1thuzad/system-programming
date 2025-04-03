#include "main.h"

int is_flag(const char *arg) {
    if (strncmp(arg, "xor", 3) == 0) {
        const char *n_str = arg + 3;
        if (strlen(n_str) == 0) return 0;
        char *end;
        long n = strtol(n_str, &end, 10);
        if (*end != '\0' || n < 2 || n > 6) return 0;
        return 1;
    } else if (strncmp(arg, "copy", 4) == 0) {
        const char *n_str = arg + 4;
        if (strlen(n_str) == 0) return 0;
        char *end;
        long n = strtol(n_str, &end, 10);
        if (*end != '\0' || n <= 0) return 0;
        return 1;
    } else if (strcmp(arg, "find") == 0 || strcmp(arg, "mask") == 0) return 1;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2)
        return throw_err(INCORRECT_ARGUMENTS);


    int i;
    for (i = 1; i < argc; i++) if (is_flag(argv[i])) break;
    if (i == argc) return throw_err(INCORRECT_ARGUMENTS);

    int input_files_count = i - 1;
    char **input_files = argv + 1;
    char *flag = argv[i];
    char **options = argv + i + 1;
    int options_count = argc - i - 1;

    if (strncmp(flag, "xor", 3) == 0) {
        if (options_count != 0) return throw_err(INCORRECT_ARGUMENTS);
        const char *n_str = flag + 3;
        int N = atoi(n_str);
        if (N < 2 || N > 6) return throw_err(OUT_OF_BOUNDS);
        for (int j = 0; j < input_files_count; j++) process_xor(input_files[j], N);
    }
    else if (strcmp(flag, "mask") == 0) {
        if (options_count != 1) return throw_err(INCORRECT_ARGUMENTS);
        unsigned int mask;
        if (sscanf(options[0], "%x", &mask) != 1) return throw_err(NOT_A_NUMBER);
        for (int j = 0; j < input_files_count; j++) process_mask(input_files[j], mask);
    }
    else if (strncmp(flag, "copy", 4) == 0) {
        if (options_count != 0) return throw_err(INCORRECT_ARGUMENTS);
        const char *n_str = flag + 4;
        int N = atoi(n_str);
        if (N <= 0) return throw_err(OUT_OF_BOUNDS);
        for (int j = 0; j < input_files_count; j++) process_copy(input_files[j], N);
    }
    else if (strcmp(flag, "find") == 0) {
        if (options_count != 1) return throw_err(INCORRECT_ARGUMENTS);
        process_find(input_files, input_files_count, options[0]);
    }
    else return throw_err(INCORRECT_OPTION);
    return 0;
}

int process_xor(const char *filename, int N) {
    FILE *file = fopen(filename, "rb");
    if (!file) return throw_err(FILE_ERROR);

    if (N == 2) {
        unsigned char xor_result = 0;
        unsigned char byte;
        while (fread(&byte, 1, 1, file) == 1) {
            xor_result ^= (byte >> 4) & 0x0F;
            xor_result ^= byte & 0x0F;
        }
        printf("File: %s XOR result: %02x\n", filename, xor_result);
    } else {
        int block_bytes = (1 << N)/8;
        switch (block_bytes) {
            case 1: {
                unsigned char result = 0;
                unsigned char buffer;
                while (fread(&buffer, 1, 1, file) == 1) result ^= buffer;
                printf("File: %s XOR result: %02x\n", filename, result);
                break;
            }
            case 2: {
                unsigned short result = 0;
                unsigned char buffer[2];
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, 2, file))) {
                    if (bytes_read < 2) memset(buffer + bytes_read, 0, 2 - bytes_read);
                    result ^= (buffer[0] << 8) | buffer[1];
                }
                printf("File: %s XOR result: %04x\n", filename, result);
                break;
            }
            case 4: {
                unsigned int result = 0;
                unsigned char buffer[4];
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, 4, file))) {
                    if (bytes_read < 4) memset(buffer + bytes_read, 0, 4 - bytes_read);
                    result ^= (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                }
                printf("File: %s XOR result: %08x\n", filename, result);
                break;
            }
            case 8: {
                unsigned long long result = 0;
                unsigned char buffer[8];
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, 8, file))) {
                    if (bytes_read < 8) memset(buffer + bytes_read, 0, 8 - bytes_read);
                    result ^= ((unsigned long long)buffer[0] << 56) |
                              ((unsigned long long)buffer[1] << 48) |
                              ((unsigned long long)buffer[2] << 40) |
                              ((unsigned long long)buffer[3] << 32) |
                              ((unsigned long long)buffer[4] << 24) |
                              ((unsigned long long)buffer[5] << 16) |
                              ((unsigned long long)buffer[6] << 8) |
                              buffer[7];
                }
                printf("File: %s XOR result: %016llx\n", filename, result);
                break;
            }
            default: fclose(file); return throw_err(INCORRECT_OPTION);
        }
    }
    fclose(file);
    return 0;
}

int process_mask(const char *filename, unsigned int mask) {
    FILE *file = fopen(filename, "rb");
    if (!file) return throw_err(FILE_ERROR);

    unsigned char buffer[4];
    size_t bytes_read;
    unsigned int count = 0;

    while ((bytes_read = fread(buffer, 1, 4, file))) {
        if (bytes_read < 4) memset(buffer + bytes_read, 0, 4 - bytes_read);
        unsigned int value = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        if (value == mask) count++;
    }

    printf("File: %s Count: %u\n", filename, count);
    fclose(file);
    return 0;
}

int process_copy(const char *filename, int N) {
    for (int i = 1; i <= N; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char *copy_name = generate_copy_name(filename, i);
            if (copy_name) {
                copy_file(filename, copy_name);
                free(copy_name);
            } else throw_err(MEMORY_NOT_ALLOCATED);
            exit(0);
        } else if (pid < 0) throw_err(FORK_ERROR);
    }
    while (wait(NULL) > 0);
    return 0;
}

char *generate_copy_name(const char *original, int copy_num) {
    const char *ext = strrchr(original, '.');
    char *new_name;
    if (ext) {
        size_t base_len = ext - original;
        size_t ext_len = strlen(ext);
        new_name = malloc(base_len + 10 + ext_len + 1);
        if (!new_name) return NULL;
        snprintf(new_name, base_len + 10 + ext_len + 1, "%.*s%d%s", (int)base_len, original, copy_num, ext);
    } else {
        size_t len = strlen(original) + 10;
        new_name = malloc(len + 1);
        if (!new_name) return NULL;
        snprintf(new_name, len + 1, "%s%d", original, copy_num);
    }
    return new_name;
}

int copy_file(const char *src, const char *dest) {
    FILE *in = fopen(src, "rb");
    if (!in) return throw_err(FILE_ERROR);

    FILE *out = fopen(dest, "wb");
    if (!out) {
        fclose(in);
        return throw_err(FILE_ERROR);
    }

    unsigned char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), in))) fwrite(buffer, 1, n, out);

    fclose(in);
    fclose(out);
    return 0;
}

int process_find(char **input_files, int input_files_count, const char *search_str) {
    int m = strlen(search_str);
    if (m == 0) return throw_err(INCORRECT_INPUT_DATA);
    return find_substr_in_files(search_str, m, input_files_count, input_files);
}

int find_substr_in_files(const char *substr, int m, int file_count, char **filenames) {
    for (int i = 0; i < file_count; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            int err = file_handler(substr, m, filenames[i]);
            exit(err);
        } else if (pid < 0) throw_err(FORK_ERROR);
    }

    int status, found_any = 0;
    while (wait(&status) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) found_any = 1;
    }

    if (!found_any) printf("Строка не найдена ни в одном файле.\n");
    return 0;
}

int file_handler(const char *key, int m, const char *filename) {
    FILE* stream = fopen(filename, "rb");
    if (!stream) return throw_err(FILE_ERROR);

    fseek(stream, 0, SEEK_END);
    long size = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(stream);
        return throw_err(MEMORY_NOT_ALLOCATED);
    }

    fread(content, 1, size, stream);
    content[size] = '\0';
    fclose(stream);

    int *table = malloc(m * sizeof(int));
    if (!table) {
        free(content);
        return throw_err(MEMORY_NOT_ALLOCATED);
    }

    shift_table(key, m, table);

    int ind = 0, found = 0;
    while (ind < size) {
        int pos = substr(content + ind, size - ind, key, m);
        if (pos == -1) break;
        ind += pos;
        printf("Found in %s: line %d, position %d\n", filename,
               line_counter(content, ind), pos_in_line(content, ind));
        ind += 1;
        found = 1;
    }

    free(content);
    free(table);
    return found ? 0 : 1;
}

int substr(const char* str, int n, const char* key, int m) {
    int *table = malloc(m * sizeof(int));
    if (!table) return -1;

    shift_table(key, m, table);
    int i = 0, j = 0;

    while (i < n) {
        if (str[i] == key[j]) {
            i++;
            j++;
            if (j == m) {
                free(table);
                return i - j;
            }
        } else {
            if (j > 0) j = table[j - 1];
            else i++;
        }
    }

    free(table);
    return -1;
}

int shift_table(const char *template, int n, int *result) {
    if (n == 0) return throw_err(INCORRECT_ARGUMENTS);
    result[0] = 0;
    int i = 1, j = 0;

    while (i < n) {
        if (template[i] == template[j]) result[i++] = ++j;
        else {
            if (j != 0) j = result[j - 1];
            else result[i++] = 0;
        }
    }
    return 0;
}

int line_counter(const char *str, int size) {
    int count = 1;
    for (int i = 0; i < size; ++i) if (str[i] == '\n') count++;
    return count;
}

int pos_in_line(const char *str, int size) {
    for (int i = size - 1; i >= 0; --i) if (str[i] == '\n') return size - i - 1;
    return size;
}
