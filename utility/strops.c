#include "strops.h"

int is_digit(char x) {
    return x >= '0' && x <= '9';
}

int is_lower(char x) {
    return x >= 'a' && x <= 'z';
}

int is_upper(char x) {
    return x >= 'A' && x <= 'Z';
}

int is_letter(char x) {
    return is_lower(x) || is_upper(x);
}

int is_alnum(char x) {
    return is_digit(x) || is_letter(x);
}

int is_special_character(char x) {
    return !is_letter(x) && !is_digit(x) && x != '\'' && x != '"' && x != '#' && x != '_';
}


int to_lower(char x) {
    if (is_upper(x))
        return x + ('a' - 'A');
    return x;
}

int digit_to_value(char x) {
    return x - '0';
}

int filter_string(const char *str, int (*filter)(char)) {
    if (!str)
        return 0;

    int i = 0;

    while (str[i] != 0) {
        if (!filter(str[i++]))
            return 0;
    }

    return 1;
}
