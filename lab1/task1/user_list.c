#include "user_list.h"

UserList *users_init() {
    UserList *userList = (UserList *) malloc(sizeof(UserList));
    if (!userList)
        return NULL;

    userList->length = 0;
    userList->capacity = 8;

    userList->users = (User **) malloc(sizeof(User*) * userList->capacity);
    if (!userList->users) {
        free(userList);
        return NULL;
    }

    return userList;
}

int user_list_resize(UserList *userList, int size_delta) {
    userList->capacity += size_delta;

    User **temp = (User **) realloc(userList->users, userList->capacity);
    if (!temp)
        return throw_err(MEMORY_NOT_ALLOCATED);

    userList->users = temp;

    return 0;
}

int user_list_extend(UserList *userList) {
    return user_list_resize(userList, userList->capacity * 2);
}

int user_list_append(UserList *userList, User *user) {
    if (userList->length + 1 >= userList->capacity)
        RET_ERR(user_list_extend(userList));

    userList->users[userList->length++] = user;

    return 0;
}

void user_list_destroy(UserList *userList) {
    if (!userList->users)
        return;

    for (int i = 0; i < userList->length; ++i) {
        free(userList->users[i]);
    }

    free(userList->users);
    userList->users = NULL;
}
