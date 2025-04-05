#include "server.h"

Directory *find_directory(Directory *head, const char *path) {
    Directory *current = head;
    while (current) {
        if (strcmp(current->path, path) == 0) return current;
        current = current->next;
    }
    return NULL;
}

Directory *add_directory(Directory **head, char *path) {
    Directory *dir = malloc(sizeof(Directory));
    if (!dir) {
        throw_err(MEMORY_NOT_ALLOCATED);
        return NULL;
    }

    dir->path = path;
    dir->files = NULL;
    dir->next = *head;
    *head = dir;
    return dir;
}

void add_file(Directory *dir, char *file) {
    FileList *current = dir->files;
    while (current) {
        if (strcmp(current->file, file) == 0) {
            free(file);
            return;
        }
        current = current->next;
    }

    FileList *new_file = malloc(sizeof(FileList));
    if (!new_file) {
        free(file);
        throw_err(MEMORY_NOT_ALLOCATED);
        return;
    }

    new_file->file = file;
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

Directory *process_paths(char *paths_data) {
    Directory *directories = NULL;
    char *path = paths_data;

    while (*path) {
        if (path[0] != '/') {
            path += strlen(path) + 1;
            continue;
        }

        char *resolved_path = realpath(path, NULL);
        if (!resolved_path) {
            path += strlen(path) + 1;
            continue;
        }

        struct stat st;
        if (stat(resolved_path, &st) == -1 || !S_ISREG(st.st_mode)) {
            free(resolved_path);
            path += strlen(path) + 1;
            continue;
        }

        char *temp_dir = strdup(resolved_path);
        char *temp_file = strdup(resolved_path);
        free(resolved_path);

        if (!temp_dir || !temp_file) {
            free(temp_dir);
            free(temp_file);
            throw_err(MEMORY_NOT_ALLOCATED);
            return NULL;
        }

        char *dir_path = dirname(temp_dir);
        char *file_name = basename(temp_file);

        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            free(temp_dir);
            free(temp_file);
            path += strlen(path) + 1;
            continue;
        }

        char *dir_copy = strdup(dir_path);
        char *file_copy = strdup(file_name);
        free(temp_dir);
        free(temp_file);

        if (!dir_copy || !file_copy) {
            free(dir_copy);
            free(file_copy);
            throw_err(MEMORY_NOT_ALLOCATED);
            return NULL;
        }

        Directory *dir = find_directory(directories, dir_copy);
        if (!dir) {
            dir = add_directory(&directories, dir_copy);
            if (!dir) {
                free(file_copy);
                return NULL;
            }
        } else {
            free(dir_copy);
        }

        add_file(dir, file_copy);
        path += strlen(path) + 1;
    }
    return directories;
}

int main() {
    key_t key_req = ftok("tokfile", 55);
    if (key_req == -1)
        throw_err(MESSAGE_QUEUE_ERROR);

    int req_queue = msgget(key_req, IPC_CREAT | 0666);
    if (req_queue == -1)
        throw_err(MESSAGE_QUEUE_ERROR);

    key_t key_res = ftok("tokfile", 56);
    if (key_res == -1)
        throw_err(MESSAGE_QUEUE_ERROR);

    int res_queue = msgget(key_res, IPC_CREAT | 0666);
    if (res_queue == -1)
        throw_err(MESSAGE_QUEUE_ERROR);

    while (1) {
        RequestMsg req;
        ssize_t received = msgrcv(req_queue, &req, sizeof(req.data), 1, 0);
        if (received == -1)
            throw_err(MESSAGE_QUEUE_ERROR);

        pid_t client_pid;
        memcpy(&client_pid, req.data, sizeof(pid_t));
        char *paths_data = req.data + sizeof(pid_t);

        Directory *directories = process_paths(paths_data);
        if (!directories) continue;

        ResponseMsg res;
        res.mtype = client_pid;
        char *ptr = res.data;
        size_t remaining = sizeof(res.data);

        for (Directory *dir = directories; dir && remaining > 0; dir = dir->next) {
            size_t dir_len = strlen(dir->path) + 1;
            if (dir_len > remaining) break;
            memcpy(ptr, dir->path, dir_len);
            ptr += dir_len;
            remaining -= dir_len;

            for (FileList *file = dir->files; file && remaining > 0; file = file->next) {
                size_t file_len = strlen(file->file) + 1;
                if (file_len > remaining) break;
                memcpy(ptr, file->file, file_len);
                ptr += file_len;
                remaining -= file_len;
            }

            if (remaining < 1) break;
            *ptr++ = '\0';
            remaining--;
        }

        if (remaining >= 1) *ptr++ = '\0';
        else ptr = res.data + sizeof(res.data) - 1;

        size_t data_size = ptr - res.data;
        if (msgsnd(res_queue, &res, data_size, 0) == -1) {
            free_directories(directories);
            return throw_err(MESSAGE_QUEUE_ERROR);
        }

        free_directories(directories);
    }
    return 0;
}
