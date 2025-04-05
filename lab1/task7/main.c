#include "main.h"

int main(int argc, char **argv) {
    if (argc < 2)
        return throw_err(INCORRECT_ARGUMENTS);

    for (int i = 1; i < argc; ++i) {
        parse_directory(argv[i]);
    }

    return 0;
}

int parse_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir)
        return throw_err(FILE_ERROR);

    printf("\nPath: %s\n", path);

    char *cur_path = getcwd(NULL, 0);

    chdir(path);

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        print_file_info(entry->d_name);
    }

    closedir(dir);

    chdir(cur_path);

    free(cur_path);
    
    return 0;
}

int print_file_info(const char *filename) {
    int desc = open(filename, O_RDONLY);
    if (desc < 0)
        return throw_err(FILE_ERROR);


    struct stat stats;

    if (fstat(desc, &stats) < 0)
        return throw_err(FILE_ERROR);

    char type;

    if (S_ISDIR(stats.st_mode))
        type = 'd';
    else if (S_ISLNK(stats.st_mode))
        type = 'l';
    else if (S_ISCHR(stats.st_mode))
        type = 'c';
    else if (S_ISBLK(stats.st_mode))
        type = 'b';
    else if (S_ISFIFO(stats.st_mode))
        type = 'p';
    else if (S_ISSOCK(stats.st_mode))
        type = 's';
    else
        type = '-';


    char *owner = get_rwx((stats.st_mode >> 6) & 0x7);
    char *group = get_rwx((stats.st_mode >> 3) & 0x7);
    char *others = get_rwx(stats.st_mode & 0x7);

    struct passwd *user = getpwuid(stats.st_uid);
    struct group *group_id = getgrgid(stats.st_gid);

    char time_str[20];
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", localtime(&stats.st_mtime));

    printf("%c%s%s%s %lu %s %s %lu %s %s\n", type, owner, group, others, stats.st_nlink, user->pw_name,
           group_id->gr_name, stats.st_size, time_str, filename);

    free(owner);
    free(group);
    free(others);

    return 0;
}

char *get_rwx(mode_t mode) {
    char *perms = malloc(sizeof(char) * 4);

    perms[0] = mode & 0x4 ? 'r' : '-';
    perms[1] = mode & 0x2 ? 'w' : '-';
    perms[2] = mode & 0x1 ? 'x' : '-';
    perms[3] = '\0';

    return perms;
}
