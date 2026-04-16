#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#define LOCKFILE "/var/run/daemon.pid"
#define CONFFILE "/etc/daemon.conf"

void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
        printf("getrlimit, errno %d", errno);
        exit(1);
    }

    if ((pid = fork()) == -1)
    {
        printf("fork, errno %d", errno);
        exit(1);
    }
    else if (pid != 0)
    {
        exit(0);
    }

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        printf("sigaction, errno %d", errno);
        exit(1);
    }

    if (setsid() == -1)
    {
        printf("setsid, errno %d", errno);
        exit(1);
    }

    if (chdir("/") == -1)
    {
        printf("chdir, errno %d", errno);
        exit(1);
    }

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;

    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog(cmd, LOG_PID | LOG_CONS, LOG_DAEMON);

    if (fd0 == -1)
    {
        syslog(LOG_ERR, "open, errno %d", errno);
        exit(1);
    }
    if (fd1 == -1)
    {
        syslog(LOG_ERR, "dup, errno %d", errno);
        exit(1);
    }
    if (fd2 == -1)
    {
        syslog(LOG_ERR, "dup, errno %d", errno);
        exit(1);
    }
}

void reread(void)
{
    FILE *fd;
    char buf[256];
    if ((fd = fopen(CONFFILE, "r")) == NULL)
    {
        syslog(LOG_ERR, "open");
    }
    while (fgets(buf, sizeof(buf), fd))
    {
        syslog(LOG_INFO, "%s", buf);
    }
}

int already_running(void)
{
    int fd;
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char buf[16];
    fd = open(LOCKFILE, O_CREAT | O_EXCL | O_RDWR, perms);

    if (fd == -1)
    {
        syslog(LOG_ERR, "open, errno %d", errno);
        exit(1);
    }

    struct flock fl;
    fl.l_type = F_RDLCK | F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "fcntl, errno %d", errno);
        exit(1);
    }

    if (ftruncate(fd, 0) == -1)
    {
        syslog(LOG_ERR, "ftruncate, errno %d", errno);
        exit(1);
    }
    sprintf(buf, "%ld", (long)getpid());
    if (write(fd, buf, strlen(buf) + 1) == -1)
    {
        syslog(LOG_ERR, "write, errno %d", errno);
        exit(1);
    }

    return 0;
}

sigset_t mask;

void *thr_fn(void *arg)
{
    int signumb;

    for (;;)
    {
        if (sigwait(&mask, &signumb) != 0)
        {
            syslog(LOG_ERR, "sigwait, errno %d", errno);
            pthread_exit(0);
        }
        switch (signumb)
        {
        case SIGHUP:
            syslog(LOG_INFO, "Signal SIGHUP received");
            reread();
            break;
        case SIGTERM:
            syslog(LOG_INFO, "Signal SIGTERM received. Exiting");
            pthread_exit(0);
        default:
            syslog(LOG_INFO, "Signal received: %d\n", signumb);
        }
    }

    pthread_exit(0);
}

int main(void)
{
    time_t cur_time;
    int err;
    pthread_t tid;
    struct sigaction sa;

    daemonize("daemon");

    if (already_running() != 0)
    {
        syslog(LOG_ERR, "already running\n");
        exit(1);
    }

    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        syslog(LOG_ERR, "sigaction, errno %d", errno);
        exit(1);
    }

    sigfillset(&mask);

    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        syslog(LOG_ERR, "pthread_sigmask, errno %d", errno);
        exit(1);
    }

    err = pthread_create(&tid, NULL, thr_fn, 0);
    if (err == -1)
    {
        syslog(LOG_ERR, "pthread_create, errno %d", errno);
        exit(1);
    }

    for (int i = 1; i < 21; i++)
    {
        kill(getpid(), i);
    }

    while (1)
    {
        cur_time = time(NULL);
        syslog(LOG_INFO, "%s", ctime(&cur_time));
        sleep(10);
    }

    return 0;
}
