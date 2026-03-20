#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

static void scan_dir(const char *path, const char *prefix) {
    struct stat statbuf;
    DIR *dp;
    struct dirent *dirp;
    char **names = NULL;
    int *is_dirs = NULL, count = 0, capacity = 0, i;
    if ((dp = opendir(path)) == NULL) {
        if (errno != ENOTDIR)
            fprintf(stderr, "Cant open dir %s: %s\n", path, strerror(errno));
        return;
    }
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;
        if (count >= capacity) {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            names = realloc(names, capacity * sizeof(char *));
            is_dirs = realloc(is_dirs, capacity * sizeof(int));
            if (!names || !is_dirs) {
                perror("realloc");
                exit(1);
            }
        }
        names[count] = strdup(dirp->d_name);
        if (!names[count]) {
            perror("strdup");
            exit(1);
        }
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, dirp->d_name);
        
        is_dirs[count] = 0;
        if (lstat(full_path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            is_dirs[count] = 1;
        }
        count++;
    }
    closedir(dp);

    for (i = 0; i < count; i++) {
        int is_last = (i == count - 1);
        printf("%s%s%s\n", prefix, is_last ? "-- " : "|-- ", names[i]);

        if (is_dirs[i]) {
            char next_prefix[PATH_MAX];
            char child_path[PATH_MAX];

            snprintf(child_path, sizeof(child_path), "%s/%s", path, names[i]);
            snprintf(next_prefix, sizeof(next_prefix), "%s%s", prefix, is_last ? "    " : "|   ");
            scan_dir(child_path, next_prefix);
        }
        free(names[i]);
    }

    free(names);
    free(is_dirs);
}

int main(int argc, char **argv) {
    const char *root_path;
    struct stat statbuf;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [path/to/dir]\n", argv[0]);
        return 1;
    }

    root_path = argv[1];
    if (lstat(root_path, &statbuf) < 0) {
        fprintf(stderr, "Ошибка доступа к %s: %s\n", root_path, strerror(errno));
        return 1;
    }
    printf("%s\n", root_path);
    scan_dir(root_path, "");

    return 0;
}