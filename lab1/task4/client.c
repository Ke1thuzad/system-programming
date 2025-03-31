#include "client.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        return throw_err(INCORRECT_ARGUMENTS);
    }

    key_t token = ftok("./tokfile", 55);

    int msg_fd = msgget(token, 0);
    if (msg_fd < 0) {
        return throw_err(MESSAGE_QUEUE_ERROR);
    }

    msg_buffer message;

    struct sockaddr_in address;

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        return throw_err(SOCKET_ERROR);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) < 0) {
        close(client_fd);
        return throw_err(INCORRECT_ARGUMENTS);
    }

    int status = connect(client_fd, (struct sockaddr *) &address, sizeof(address));
    if (status < 0) {
        close(client_fd);
        return throw_err(SOCKET_ERROR);
    }

    message.msgtype = 1;

    size_t current_read = msgrcv(msg_fd, &message, sizeof(message) - sizeof(long), 1, 0);
    if (current_read < 1) {
        close(client_fd);
        return throw_err(MESSAGE_QUEUE_ERROR);
    }


    FILE *file = fopen(argv[1], "r");
    if (!file) {
        close(client_fd);
        return throw_err(FILE_ERROR);
    }

    char *command;

    int command_id, argument;

    while (!feof(file)) {
        command = get_command(file);
        if (!command) {
            close(client_fd);
            return throw_err(FILE_ERROR);
        }

        if (parse_command(command, &command_id, &argument) < 0) {
            printf("Incorrect command!\n");

            free(command);
        } else {
//            if (msgrcv(msg_fd, &message, sizeof(message) - sizeof(long), 0, IPC_NOWAIT)) {
//                if (message.msgtype == 2) {
//                    break;
//                }
//            }

            message.command = command_id;
            message.argument = argument;
            message.msgtype = 1;

            free(command);

            if (msgsnd(msg_fd, &message, sizeof(message) - sizeof(long), 0) < 0) {
                close(client_fd);
                return throw_err(MESSAGE_QUEUE_ERROR);
            }

        }
    }

    message.msgtype = 2;

    if (msgsnd(msg_fd, &message, sizeof(message) - sizeof(long), 0) < 0) {
        close(client_fd);
        return throw_err(MESSAGE_QUEUE_ERROR);
    }


    close(client_fd);

    return 0;
}

char* get_command(FILE *file) {
    if (!file)
        return NULL;

    char * result = (char *) calloc(MAX_LENGTH, sizeof(char));
    if (!result)
        return NULL;

    for (int i = 0; i < 12; ++i) {
        int ch = fgetc(file);
        if (ch < ' ') {
            break;
        }

        result[i] = (char) ch;
    }

    return result;
}

int parse_command(char *str, int *cmd, int *argument) {
    char *command = strtok(str, " ");

    if (!command) {
        *cmd = -1;
        return -1;
    }

    if (strcmp(command, "take") == 0) {
        *cmd = 1;

        char *arg = strtok(NULL, " ");
        if (!arg) {
            *argument = -1;
            return -1;
        }
        if (strcmp(arg, "wolf") == 0) {
            *argument = 1;
        } else if (strcmp(arg, "goat") == 0) {
            *argument = 2;
        } else if (strcmp(arg, "cabbage") == 0) {
            *argument = 3;
        } else {
            *argument = -1;
        }
    } else if (strcmp(command, "put") == 0) {
        *cmd = 2;
    } else if (strcmp(command, "move") == 0) {
        *cmd = 3;
    } else {
        *cmd = -1;
        return -1;
    }

    return 0;
}
