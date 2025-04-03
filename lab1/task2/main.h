#ifndef SYSTEM_PROGRAMMING_MAIN_H
#define SYSTEM_PROGRAMMING_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <error_handler.h>

int process_xor(const char *filename, int N);
int process_mask(const char *filename, unsigned int mask);
int process_copy(const char *filename, int N);
int process_find(char **input_files, int input_files_count, const char *search_str);
char *generate_copy_name(const char *original, int copy_num);
int copy_file(const char *src, const char *dest);
int substr(const char* str, int n, const char* key, int m);
int shift_table(const char *template, int n, int *result);
int line_counter(const char *str, int size);
int pos_in_line(const char *str, int size);
int find_substr_in_files(const char *substr, int m, int file_count, char **filenames);
int file_handler(const char *key, int m, const char *filename);

#endif //SYSTEM_PROGRAMMING_MAIN_H
