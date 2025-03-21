#include "main.h"

int main() {
    User *user = (User *) malloc(sizeof(User));
    if (!user)
        return throw_err(MEMORY_NOT_ALLOCATED);

    register_user(user);

    free(user);

    return 0;
}

int register_user(User *result) {
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

    return 0;
}

int login_user(User **users, int size) {
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

    for (int i = 0; i < size; ++i) {
        if (strcmp(users[i]->login, login) == 0 && users[i]->pin == pin) {
            printf("***Log-in attempt is successful!***");
            return 1;
        }
    }

    printf("***Incorrect login or pin!***");

    return 0;
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

void skip_to_nl(FILE* stream) {
    char character;

    do {
        character = (char) fgetc(stream);
    } while (character >= ' ');
}
