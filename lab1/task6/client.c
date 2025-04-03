#include "client.h"


int main(int argc, char *argv[]) {
    if (argc != 2)
        return 1;


    FILE *file = fopen(argv[1], "r");
    if (!file) throw_err(FILE_ERROR);

    key_t key_req = ftok("tokfile", 55);
    if (key_req == -1) {
        fclose(file);
        throw_err(MESSAGE_QUEUE_ERROR);
    }

    int req_queue = msgget(key_req, 0666);
    if (req_queue == -1) {
        fclose(file);
        throw_err(MESSAGE_QUEUE_ERROR);
    }

    key_t key_res = ftok("tokfile", 56);
    if (key_res == -1) {
        fclose(file);
        throw_err(MESSAGE_QUEUE_ERROR);
    }

    int res_queue = msgget(key_res, 0666);
    if (res_queue == -1) {
        fclose(file);
        throw_err(MESSAGE_QUEUE_ERROR);
    }

    RequestMsg req;
    req.mtype = 1;

    pid_t pid = getpid();
    memcpy(req.data, &pid, sizeof(pid_t));

    char *ptr = req.data + sizeof(pid_t);
    size_t remaining = MAX_MSG_SIZE - sizeof(pid_t);

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (line[0] != '/') {
            fprintf(stderr, "Path is not absolute: %s\n", line);
            fclose(file);
            return 1;
        }

        size_t len = strlen(line) + 1;
        if (remaining < len) {
            fprintf(stderr, "Message too long\n");
            fclose(file);
            return 1;
        }

        strcpy(ptr, line);
        ptr += len;
        remaining -= len;
    }
    fclose(file);

    *ptr++ = '\0';
    *ptr = '\0';

    if (msgsnd(req_queue, &req, sizeof(req.data), 0) == -1)
        throw_err(MESSAGE_QUEUE_ERROR);

    ResponseMsg res;
    ssize_t bytes = msgrcv(res_queue, &res, sizeof(res.data), pid, 0);
    if (bytes == -1) throw_err(MESSAGE_QUEUE_ERROR);

    char *data = res.data;
    while (*data) {
        char *dir = data;
        printf("\nDirectory: %s\nFiles:", dir);
        data += strlen(dir) + 1;

        while (*data) {
            char *filename = data;
            printf(" %s", filename);
            data += strlen(filename) + 1;
        }
        data++;
    }
    printf("\n");
    return 0;
}
