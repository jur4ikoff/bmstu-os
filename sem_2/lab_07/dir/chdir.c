#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#ifdef PATH_MAX
long pathmax = PATH_MAX;
#else
long pathmax = 0;
#endif
long posix_version = 0;
long xsi_version = 0;
#define PATH_MAX_GUESS 1024

char *path_alloc(size_t *sizep)
{
    char *ptr;
    size_t size;
    if (posix_version == 0)
        posix_version = sysconf(_SC_VERSION);
    if (xsi_version == 0)
        xsi_version = sysconf(_SC_XOPEN_VERSION);
    if (pathmax == 0)
    {
        errno = 0;
        if ((pathmax = pathconf("/", _PC_PATH_MAX)) < 0)
        {
            if (errno == 0)
                pathmax = PATH_MAX_GUESS;
            else
            {
                perror("pathconf");
                printf("errno: %d\n", errno);
                exit(1);
            }
        }
        else
        {
            pathmax++;
        }
    }
    if ((posix_version < 200112L) && (xsi_version < 4))
        size = pathmax + 1;
    else
        size = pathmax;
    if ((ptr = malloc(size)) == NULL)
    {
        perror("malloc");
        exit(1);
    }
    if (sizep != NULL)
        *sizep = size;
    return (ptr);
}

typedef int Myfunc(const char *, const struct stat *, int, int);
Myfunc myfunc;
int myftw(char *, Myfunc *);
int dopath(Myfunc *, int);

int main(int argc, char *argv[])
{
    int ret;
    if (argc != 2)
    {
        printf("Использование: ftw <начальный_каталог>");
        exit(1);
    }

    printf("%s\n", argv[1]);
    ret = myftw(argv[1], myfunc);
    exit(ret);
}

#define FTW_F 1
#define FTW_D 2
#define FTW_DNR 3
#define FTW_NS 4
char *fullpath;
size_t pathlen;

int myftw(char *pathname, Myfunc *func)
{
    size_t len;
    fullpath = path_alloc(&len);
    if (pathlen <= strlen(pathname))
    {
        pathlen = strlen(pathname) * 2;
        if ((fullpath = realloc(fullpath, pathlen)) == NULL)
        {
            perror("realloc");
            exit(1);
        }
    }
    strcpy(fullpath, pathname);
    return (dopath(func, 0));
}

int dopath(Myfunc *func, int depth)
{
    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;
    int ret;
    size_t n;

    if (lstat(fullpath, &statbuf) < 0)
        return (func(fullpath, &statbuf, FTW_NS, depth));
    if (S_ISDIR(statbuf.st_mode) == 0)
        return (func(fullpath, &statbuf, FTW_F, depth));

    if ((ret = func(fullpath, &statbuf, FTW_D, depth)) != 0)
        return (ret);

    n = strlen(fullpath);
    if (n + NAME_MAX + 2 > pathlen)
    {
        pathlen *= 2;
        if ((fullpath = realloc(fullpath, pathlen)) == NULL)
        {
            perror("realloc");
            exit(1);
        }
    }
    fullpath[n++] = '/';
    fullpath[n] = 0;

    if ((dp = opendir(fullpath)) == NULL)
        return (func(fullpath, &statbuf, FTW_DNR, depth));

    if (chdir(fullpath) == -1)
    {
        perror("chdir");
        exit(1);
    }

    while (ret == 0 && (dirp = readdir(dp)) != NULL)
    {
        if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
        {
            strcpy(fullpath, dirp->d_name);
            ret = dopath(func, depth + 1);
        }
    }

    if (chdir("..") == -1)
    {
        perror("chdir");
        exit(1);
    }
    if (closedir(dp) == -1)
    {
        perror("closedir");
        exit(1);
    }
    return ret;
}

int myfunc(const char *pathname, const struct stat *statptr, int type, int depth)
{
    const char *name = strrchr(pathname, '/');
    name = name ? name + 1 : pathname;

    for (int i = 0; i < depth; i++)
    {
        printf("|   ");
    }
    if (depth > 0)
    {
        printf("|--- ");
    }
    printf("%s", name);
    printf("\n");

    switch (type)
    {
        case FTW_DNR:
            printf("закрыт доступ к каталогу %s", pathname);
            break;
        case FTW_NS:
            printf("ошибка вызова функции stat для %s", pathname);
            break;
        default:
            break;
    }
    return 0;
}

// ________________________________________________________
// Executed in   12.06 millis    fish           external
//    usr time    2.31 millis    0.28 millis    2.03 millis
//    sys time    6.72 millis    1.49 millis    5.23 millis
