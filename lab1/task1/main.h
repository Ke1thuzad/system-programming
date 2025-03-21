#ifndef SYSTEM_PROGRAMMING_MAIN_H
#define SYSTEM_PROGRAMMING_MAIN_H

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <error_handler.h>
#include <strops.h>

typedef struct User {
    int id;
    char username[64];
    char login[6];
    int pin;
    int limit;
} User;

int register_user(User *result);

int nread_value_str(int *result, int n);
void skip_to_nl(FILE* stream);

#endif //SYSTEM_PROGRAMMING_MAIN_H
