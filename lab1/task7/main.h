#ifndef SYSTEM_PROGRAMMING_MAIN_H
#define SYSTEM_PROGRAMMING_MAIN_H

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <error_handler.h>

int parse_directory(const char *path);
int print_file_info(const char *filename);
char *get_rwx(mode_t mode);

#endif //SYSTEM_PROGRAMMING_MAIN_H
