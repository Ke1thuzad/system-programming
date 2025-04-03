#include "server.h"

Directory* find_directory(Directory *head, const char *path) {
    Directory *current = head;
    while (current) {
        if (strcmp(current->path, path) == 0) return current;
        current = current->next;
    }
    return NULL;
}

Directory* add_directory(Directory **head, const char *path) {
    Directory *dir = malloc(sizeof(Directory));
    if (!dir) throw_err(MEMORY_NOT_ALLOCATED);

    dir->path = strdup(path);
    if (!dir->path) {
        free(dir);
        throw_err(MEMORY_NOT_ALLOCATED);
    }

    dir->files = NULL;
    dir->next = *head;
    *head = dir;
    return dir;
}

void add_file(Directory *dir, const char *file) {
    FileList *current = dir->files;
    while (current) {
        if (strcmp(current->file, file) == 0) return;
        current = current->next;
    }

    FileList *new_file = malloc(sizeof(FileList));
    if (!new_file) throw_err(MEMORY_NOT_ALLOCATED);

    new_file->file = strdup(file);
    if (!new_file->file) {
        free(new_file);
        throw_err(MEMORY_NOT_ALLOCATED);
    }

    new_file->next = dir->files;
    dir->files = new_file;
}

void free_directories(Directory *head) {
    while (head) {
        Directory *next = head->next;
        free(head->path);

        FileList *file = head->files;
        while (file) {
            FileList *next_file = file->next;
            free(file->file);
            free(file);
            file = next_file;
        }

        free(head);
        head = next;
    }
}

Directory* process_paths(char *paths_data) {
    Directory *directories = NULL;
    char *path = paths_data;

    while (*path) {
        if (path[0] != '/') {
            path += strlen(path) + 1;
            continue;
        }

        // Получаем абсолютный путь
        char resolved_path[1024];
        if (!realpath(path, resolved_path)) {
            path += strlen(path) + 1;
            continue;
        }

        // Создаем временную копию для dirname/basename
        char *temp_path = strdup(resolved_path);
        if (!temp_path) throw_err(MEMORY_NOT_ALLOCATED);

        // Разделяем путь
        char *dir_path = dirname(temp_path);
        char *file_name = basename(temp_path);

        // Фильтруем . и ..
        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            free(temp_path);
            path += strlen(path) + 1;
            continue;
        }

        // Создаем копии для хранения в структурах
        char *dir_copy = strdup(dir_path);
        char *file_copy = strdup(file_name);
        if (!dir_copy || !file_copy) {
            free(temp_path);
            free(dir_copy);
            free(file_copy);
            throw_err(MEMORY_NOT_ALLOCATED);
        }

        // Добавляем в структуры
        Directory *dir = find_directory(directories, dir_copy);
        if (!dir) {
            dir = add_directory(&directories, dir_copy);
        }
        add_file(dir, file_copy);

        // Освобождаем временные ресурсы
        free(temp_path);
        free(dir_copy);
        free(file_copy);

        path += strlen(path) + 1;
    }
    return directories;
}

int main() {
    key_t key_req = ftok("tokfile", 55);
    if (key_req == -1) throw_err(MESSAGE_QUEUE_ERROR);

    int req_queue = msgget(key_req, IPC_CREAT | 0666);
    if (req_queue == -1) throw_err(MESSAGE_QUEUE_ERROR);

    key_t key_res = ftok("tokfile", 56);
    if (key_res == -1) throw_err(MESSAGE_QUEUE_ERROR);

    int res_queue = msgget(key_res, IPC_CREAT | 0666);
    if (res_queue == -1) throw_err(MESSAGE_QUEUE_ERROR);

    while (1) {
        RequestMsg req;
        if (msgrcv(req_queue, &req, sizeof(req.data), 1, 0) == -1)
            throw_err(MESSAGE_QUEUE_ERROR);

        pid_t client_pid;
        memcpy(&client_pid, req.data, sizeof(pid_t));
        char *paths_data = req.data + sizeof(pid_t);

        Directory *directories = process_paths(paths_data);

        ResponseMsg res;
        res.mtype = client_pid;
        char *ptr = res.data;

        for (Directory *dir = directories; dir; dir = dir->next) {
            size_t dir_len = strlen(dir->path) + 1;
            memcpy(ptr, dir->path, dir_len);
            ptr += dir_len;

            for (FileList *file = dir->files; file; file = file->next) {
                size_t file_len = strlen(file->file) + 1;
                memcpy(ptr, file->file, file_len);
                ptr += file_len;
            }

            *ptr++ = '\0';
        }
        *ptr++ = '\0';

        if (msgsnd(res_queue, &res, ptr - res.data, 0) == -1) {
            free_directories(directories);
            throw_err(MESSAGE_QUEUE_ERROR);
        }

        free_directories(directories);
    }
    return 0;
}
