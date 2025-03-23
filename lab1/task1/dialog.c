#include "dialog.h"

int main_menu() {
    UserList *userList = users_init();
    if (!userList)
        return throw_err(MEMORY_NOT_ALLOCATED);

    User *current = (User *) malloc(sizeof(User));
    if (!current)
        return throw_err(MEMORY_NOT_ALLOCATED);

    register_user(current);

    RET_ERR_CB(user_list_append(userList, current), user_list_destroy(userList));

    Command cmd;

    while (1) {
        if (wait_command(&cmd)) {
            break;
        }
        if (current) {
            if (cmd == LOGOUT) {
                current = NULL;
                printf("***Logged out!***\n");
                continue;
            }

            if (check_ban(current)) {
                printf("Your command limit has ended!\n");
            } else {
                handle_command(cmd, userList);
            }
        } else {
            printf("You're not logged in!\nUse: \n\t1. 'register' \n\t2. 'login'\n");
            switch ((int) cmd) {
                case 0:
                    current = (User *) malloc(sizeof(User));

                    register_user(current);
                    RET_ERR_CB(user_list_append(userList, current), user_list_destroy(userList));
                    break;
                case 1:
                    login_user(userList);
                    break;
            }
        }
    }

    user_list_destroy(userList);

    return 0;
}

int wait_command(Command *result) {
    Array buf;
    int err = create_arr(5, &buf);
    if (err)
        return err;

    while (1) {
        printf("> ");
        err = read_whole_input(&buf);
        if (err) {
            destroy(&buf);
            return err;
        }

        printf("\n");

        if (!strcmp(buf.val, "^Z") || buf.val[0] == EOF) {
            destroy(&buf);
            return 1;
        } else if (!strcmp(buf.val, "help")) {
            *result = HELP;
            break;
        } else if (!strcmp(buf.val, "time")) {
            *result = TIME;
            break;
        } else if (!strcmp(buf.val, "date")) {
            *result = DATE;
            break;
        } else if (!strcmp(buf.val, "howmuch")) {
            *result = TIMEDIFF;
            break;
        } else if (!strcmp(buf.val, "logout")) {
            *result = LOGOUT;
            break;
        } else if (!strcmp(buf.val, "sanctions")) {
            *result = BAN;
            break;
        } else if (is_digit(buf.val[0]) && buf.length == 1) {
            *result = buf.val[0] - '0' - 1;
            break;
        } else {
            printf("Incorrect command. Please try again or use 'help' command.\n");
        }
        reset(&buf);
    }

    destroy(&buf);

    return 0;
}

int handle_command(Command cmd, UserList *userList) {
    char username[65];
    int limit = 0;

    switch (cmd) {
        case HELP:
            help_cmd();
            break;
        case TIME:
            get_time();
            break;
        case DATE:
            get_date();
            break;
        case TIMEDIFF: {
            char iso_time[20];
            char flag[3];
            printf("Enter ISO time (YYYY-MM-DDTHH:MM:SS): ");
            scanf("%19s", iso_time);

            struct tm tm;
            memset(&tm, 0, sizeof(struct tm));

            if (!ISO8601_to_Time(iso_time, &tm)) {
                throw_err(INCORRECT_ARGUMENTS);
                return 0;
            }

            skip_to_nl(stdin);

            printf("Enter flag (-s, -m, -h, -y): ");
            scanf("%2s", flag);
            skip_to_nl(stdin);

            time_flag tflag;
            if (strcmp(flag, "-s") == 0) {
                tflag = SECONDS;
            } else if (strcmp(flag, "-m") == 0) {
                tflag = MINUTES;
            } else if (strcmp(flag, "-h") == 0) {
                tflag = HOURS;
            } else if (strcmp(flag, "-y") == 0) {
                tflag = YEARS;
            } else {
                throw_err(INCORRECT_ARGUMENTS);
                return 0;
            }

            time_t initial_time = mktime(&tm);

            how_much_time_passed(initial_time, tflag);

            break;
        }
        case LOGOUT:
            break;
        case BAN:
//            while ((ch = fgetc(stdin)))
            scanf("%64s", username);
            skip_to_nl(stdin);

            if (nread_value_str(&limit, 9)) {
                throw_err(INCORRECT_ARGUMENTS);
                return 0;
            }

            if (ban_user_limit(userList, username, limit)) {
                printf("User %s has been banned successfully for %d commands!\n", username, limit);
            } else {
                printf("User %s does not exist!\n", username);
            }
            break;
    }

    return 0;
}

void cmd_description(Command cmd) {
    int level = 1;
    int j = 1;

    char *cmd_descriptions[][10] = {
            {"'help' - Print all available commands and their options",    NULL},
            {"'time' - Show current time",                                 NULL},
            {"'date' - Show current date",                                 NULL},
            {"'howmuch <date> <flag>' - How much time has passed since given date with given accuracy",
                                                                                 "-s = seconds", "-m = minutes", "-h = hours", "-y = years", NULL},
            {"'logout' - Log out of account",                              NULL},
            {"'sanctions <username> <cmd limit>' - Limit user's commands", "Cmd limit must be 9 digits or less", NULL}
    };

    printf("%*d. %s.\n", level * 2, cmd + 1, cmd_descriptions[cmd][0]);
    level++;
    while (cmd_descriptions[cmd][j] != NULL) {
        printf("%*d) %s.\n", level * 2, j, cmd_descriptions[cmd][j]);

        j++;
    }

}

void help_cmd() {
    int cmd_amount = 6;

    printf("Available commands (you can use them by typing numbers corresponding or command strings):\n");
    for (int i = 1; i <= cmd_amount; ++i) {
        cmd_description(i - 1);
    }
}

void register_user(User *result) {
    static int signed_up_count = 0;

    printf("***Registration process***\n");
    printf("Please enter nickname (64 characters max):");

    int current_input;

    char buf[65] = "";

    current_input = scanf("%64s", buf);
    while (!current_input) {
        printf("Entered data is not correct\n");
        printf("Please enter nickname again (64 characters max, others are truncated):");
        current_input = scanf("%64s", buf);
        skip_to_nl(stdin);
    }

    strncpy(result->username, buf, 64);

    buf[0] = '\0';

    skip_to_nl(stdin);

    printf("Please enter login (alphanumeric, 6 characters max, others are truncated): ");

    current_input = scanf("%6s", buf);
    while (!current_input || !filter_string(buf, is_alnum)) {
        printf("Entered data is not correct\n");
        printf("Please enter login again (alphanumeric, 6 characters max): ");
        current_input = scanf("%6s", buf);
        skip_to_nl(stdin);
    }

    strncpy(result->login, buf, 6);

    skip_to_nl(stdin);

    printf("Please enter PIN-code (0-100000): ");

    while (nread_value_str(&result->pin, 6) == 1 || result->pin > 100000) {
        printf("Entered data is not correct\n");
        printf("Please enter PIN-code again (0-100000): ");
        skip_to_nl(stdin);
    }

    result->id = signed_up_count++;

    result->limit = -1;

    printf("***Registration successful!***\n");
}

int login_user(UserList *userList) {
    printf("***Logging-in attempt***");
    printf("Login: ");

    int current_input;

    char login[7] = "";

    current_input = scanf("%6s", login);
    if (!current_input || fgetc(stdin) != '\n')
        return throw_err(INCORRECT_INPUT_DATA);

    skip_to_nl(stdin);

    int pin;

    printf("PIN-code: ");

    if (nread_value_str(&pin, 6) == 1 || pin > 100000)
        return throw_err(INCORRECT_INPUT_DATA);

    skip_to_nl(stdin);

    for (int i = 0; i < userList->length; ++i) {
        if (strcmp(userList->users[i]->login, login) == 0 && userList->users[i]->pin == pin) {
            printf("***Log-in attempt is successful!***");

            return 1;
        }
    }

    printf("***Incorrect login or pin!***");

    return 0;
}

int get_time() {
    time_t cur_time;
    time(&cur_time);

    struct tm *time_info;
    time_info = localtime(&cur_time);

    printf("Current time: %.2d:%.2d:%.2d\n", time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

    return 0;
}

int get_date() {
    time_t cur_time;
    time(&cur_time);

    struct tm *time_info;
    time_info = localtime(&cur_time);

    printf("Current date: %02d:%02d:%04d\n", time_info->tm_mday, time_info->tm_mon, 1900 + time_info->tm_year);

    return 0;
}

int how_much_time_passed(time_t initial, time_flag flag) {
    time_t cur_time;
    time(&cur_time);

    double diff = difftime(cur_time, initial);
    if (diff < 0)
        return throw_err(INCORRECT_ARGUMENTS);

    switch (flag) {
        case SECONDS:
            printf("%f seconds ", diff);
            break;
        case MINUTES:
            diff /= 60;

            printf("%f minutes ", diff);
            break;
        case HOURS:
            diff /= 3600;

            printf("%f hours ", diff);
            break;
        case YEARS:
            printf("%d years ", localtime(&initial)->tm_year - localtime(&cur_time)->tm_year);
            break;
    }

    printf("has passed since.\n");

    return 0;
}

int ban_user_limit(UserList *userList, char *username, int limit) {
    for (int i = 0; i < userList->length; ++i) {
        if (strcmp(userList->users[i]->username, username) == 0) {
            userList->users[i]->limit = limit;

            return 1;
        }
    }

    return 0;
}

int check_ban(User *user) {
    if (user->limit == -1 || user->limit > 0)
        return 0;
    return 1;
}

int nread_value_str(int *result, int n) {
    char character;
    *result = 0;

    for (int i = 0; i < n; ++i) {
        character = (char) fgetc(stdin);

        if (character <= 0 || character == '\n')
            return EOF;
        if (!is_digit(character))
            return 1;

        *result *= 10;
        *result += digit_to_value((char) character);
    }

    return 0;
}

void skip_to_nl(FILE *stream) {
    char character;

    do {
        character = (char) fgetc(stream);
    } while (character >= ' ');
}

int is_leap_year(int year) {
    if (year % 400 == 0)
        return 1;
    if (year % 100 == 0)
        return 0;
    if (year % 4 == 0)
        return 1;
    return 0;
}

int days_in_month(int month, int year) {
    if (month < 1 || month > 12)
        return 0;
    if (month == 2)
        return is_leap_year(year) ? 29 : 28;
    if (month == 4 || month == 6 || month == 9 || month == 11)
        return 30;
    return 31;
}

int ISO8601_to_Time(const char *input, struct tm *res) {
    if (input == NULL || res == NULL)
        return 0;

    if (sscanf(input, "%4d-%2d-%2dT%2d:%2d:%2d",
               &res->tm_year, &res->tm_mon, &res->tm_mday,
               &res->tm_hour, &res->tm_min, &res->tm_sec) != 6)
        return 0;

    res->tm_year -= 1900;
    res->tm_mon -= 1;

    if (res->tm_mon < 0 || res->tm_mon > 11)
        return 0;

    int days = days_in_month(res->tm_mon + 1, res->tm_year + 1900);
    if (days == 0 || res->tm_mday < 1 || res->tm_mday > days)
        return 0;

    if (res->tm_hour < 0 || res->tm_hour > 23)
        return 0;

    if (res->tm_min < 0 || res->tm_min > 59)
        return 0;

    if (res->tm_sec < 0 || res->tm_sec > 59)
        return 0;

    return 1;
}
