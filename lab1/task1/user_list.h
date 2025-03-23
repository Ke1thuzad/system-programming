#ifndef SYSTEM_PROGRAMMING_USER_LIST_H
#define SYSTEM_PROGRAMMING_USER_LIST_H

#include <malloc.h>
#include <error_handler.h>


typedef struct User {
    int id;
    char username[64];
    char login[6];
    int pin;
    int limit;
} User;

typedef struct UserList {
    User **users;
    int length;
    int capacity;
} UserList;

UserList *users_init();
int user_list_resize(UserList *userList, int size_delta);
int user_list_extend(UserList *userList);
int user_list_append(UserList *userList, User *user);
void user_list_destroy(UserList *userList);

#endif //SYSTEM_PROGRAMMING_USER_LIST_H
