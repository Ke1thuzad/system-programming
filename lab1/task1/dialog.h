#ifndef SYSTEM_PROGRAMMING_DIALOG_H
#define SYSTEM_PROGRAMMING_DIALOG_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <strops.h>
#include <arrays/array.h>

#include "user_list.h"

typedef enum time_flag {
    SECONDS,
    MINUTES,
    HOURS,
    YEARS
} time_flag;

typedef enum Command {
    HELP,
    TIME,
    DATE,
    TIMEDIFF,
    LOGOUT,
    BAN
} Command;


int main_menu();
int handle_command(Command cmd, UserList *userList);
int wait_command(Command *result);
void cmd_description(Command cmd);
void help_cmd();

void register_user(User *result, UserList *users);
User *login_user(UserList *userList);

int get_time();
int get_date();
int how_much_time_passed(time_t initial, time_flag flag);
int ban_user_limit(UserList *userList, char *username, int limit);
int check_ban(User *user);

int nread_value_str(int *result, int n);
void skip_to_nl(FILE* stream);

int is_leap_year(int year);
int days_in_month(int month, int year);
int ISO8601_to_Time(const char *input, struct tm *res);


#endif //SYSTEM_PROGRAMMING_DIALOG_H
