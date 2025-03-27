#include "server.h"

volatile sig_atomic_t keep_running = 1;

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct sockaddr_in address;

    socklen_t addrlen = sizeof(address);

    int opt = 1;
    int client_fd;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return throw_err(SOCKET_ERROR);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        return throw_err(SOCKET_ERROR);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        close(server_fd);
        return throw_err(SOCKET_ERROR);
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        return throw_err(SOCKET_ERROR);
    }

    printf("Listening to commands!\n");

    char buf[16], *command, *item;
    size_t current_read;


    // Game description

    int bank = 0;  // 0 - left, 1 - right
    Item current_item = EMPTY;  // 0 - empty, 1 - wolf, 2 - goat, 3 - cabbage
    int bank_inventories[2][3] = {{1, 1, 1},
                                  {0, 0, 0}};  // pos 0 - wolf, pos 1 - goat, pos 2 - cabbage

    //


    while (keep_running) {
        client_fd = accept(server_fd, (struct sockaddr *) &address, &addrlen);
        if (client_fd < 0) {
            throw_err(SOCKET_ERROR);
            continue;
        }

        current_read = read(client_fd, buf, 14);
        close(client_fd);
        if (current_read < 3) {
            buf[0] = '\0';
            continue;
        }

        // TODO: auth

        command = strtok(buf, " ");

        if (strcmp(command, "take") == 0) {
            if (current_item)
                put_item(bank_inventories[bank], &current_item);

            item = strtok(NULL, " ");

            if (strcmp(item, "wolf") == 0) {
                bank_inventories[bank][0] = 0;
                current_item = WOLF;
            } else if (strcmp(item, "goat") == 0) {
                bank_inventories[bank][1] = 0;
                current_item = GOAT;
            } else if (strcmp(item, "cabbage") == 0) {
                bank_inventories[bank][2] = 0;
                current_item = CABBAGE;
            } else {
                printf("Unknown item!\n");
            }
        } else if (strcmp(command, "put") == 0) {
            put_item(bank_inventories[bank], &current_item);
            if (check_conflicts(bank_inventories[bank])) {
                printf("Conflict! Game is resetting!\n");
                bank = 0;
                current_item = EMPTY;

                bank_inventories[0][0] = 1;
                bank_inventories[0][1] = 1;
                bank_inventories[0][2] = 1;

                bank_inventories[1][0] = 0;
                bank_inventories[1][1] = 0;
                bank_inventories[1][2] = 0;
            }
        } else if (strcmp(command, "move") == 0) {
            bank = !bank;
        } else {
            printf("Unknown command!\n");
        }
    }

    buf[0] = '\0';

    close(server_fd);

    return 0;
}

void handle_signal() {
    keep_running = 0;
    printf("Server shut down!\n");
}

int check_conflicts(const int *inventory) {
    return !(inventory[0] && inventory[1]) || !(inventory[1] && inventory[2]);
}

void put_item(int *inventory, Item *item) {
    switch (*item) {
        case EMPTY:
            break;
        case WOLF:
            inventory[0] = 1;
            printf("Put down wolf!\n");
            break;
        case GOAT:
            inventory[1] = 1;
            printf("Put down goat!\n");
            break;
        case CABBAGE:
            inventory[2] = 1;
            printf("Put down cabbage!\n");
            break;
    }

    *item = EMPTY;
}
