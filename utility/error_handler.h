#ifndef FUNDAMENTAL_ALGORITHMS_ERROR_HANDLER_H
#define FUNDAMENTAL_ALGORITHMS_ERROR_HANDLER_H

#include <stdio.h>
#include <stdlib.h>

#define RET_ERR(x) do {int err = (x); if (err) return err;} while(0)
#define RET_ERR_CB(x, cb) do {int err = (x); if (err) {(cb); return err;}} while(0)

typedef enum errcode {
    OUT_OF_BOUNDS = 2,
    NOT_A_NUMBER,
    INCORRECT_ARGUMENTS,
    INCORRECT_OPTION,
    MEMORY_NOT_ALLOCATED,
    FILE_ERROR,
    INCORRECT_INPUT_DATA,
    DIVISION_BY_ZERO,
    SEMAPHORE_ERROR,
    SOCKET_ERROR,
    MESSAGE_QUEUE_ERROR,
    FORK_ERROR
} errcode;

int throw_err(errcode err);

#endif //FUNDAMENTAL_ALGORITHMS_ERROR_HANDLER_H
