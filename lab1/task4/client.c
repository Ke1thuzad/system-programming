#include "client.h"

int main() {
    struct sockaddr_in address;

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        return throw_err(SOCKET_ERROR);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) < 0) {
        return throw_err(INCORRECT_ARGUMENTS);
    }

    int status = connect(client_fd, (struct sockaddr *) &address, sizeof(address));
    if (status < 0) {
        return throw_err(SOCKET_ERROR);
    }



    close(client_fd);

    return 0;
}
