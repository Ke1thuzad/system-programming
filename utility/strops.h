#pragma once

#ifndef SYSTEM_PROGRAMMING_STROPS_H
#define SYSTEM_PROGRAMMING_STROPS_H

int is_digit(char x);
int is_lower(char x);
int is_upper(char x);
int is_letter(char x);
int is_alnum(char x);
int is_special_character(char x);

int to_lower(char x);
int digit_to_value(char x);
int filter_string(const char *str, int (*filter)(char));

#endif //SYSTEM_PROGRAMMING_STROPS_H
