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

#define CONFFILE "/etc/daemon.conf"
#define LOCKFILE "/var/run/daemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

sigset_t mask;

int already_running(void)
{
	int fd;
	char buf[16];
	struct flock fl;

	fd = open(LOCKFILE, O_CREAT | O_EXCL | O_RDWR, LOCKMODE);
	if (fd == -1)
	{
		syslog(LOG_ERR, "open %s: %s", LOCKFILE, strerror(errno));
		exit(1);
	}

	fl.l_type = F_WRLCK;
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
		syslog(LOG_ERR, "lockfile '%s': %s", LOCKFILE, strerror(errno));
		exit(1);
	}

	ftruncate(fd, 0);
	sprintf(buf, "%d", getpid());
	write(fd, buf, strlen(buf) + 1);
	return 0;
}

void daemonize(const char *cmd)
{
	int i, fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rl;
	struct sigaction sa;

	umask(0);

	if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
	{
		perror("getrlimit");
		exit(1);
	}

	if ((pid = fork()) == -1)
	{
		perror("fork");
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
	{
		rl.rlim_max = 1024;
	}

	for (i = 0; i < rl.rlim_max; i++)
	{
		close(i);
	}

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	openlog(cmd, LOG_CONS, LOG_DAEMON);

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
	char data[256];

	if ((fd = fopen(CONFFILE, "r")) == NULL)
	{
		syslog(LOG_ERR, "fopen %s : %s", CONFFILE, strerror(errno));
		exit(1);
	}

	fscanf(fd, "%s", data);
	fclose(fd);
	syslog(LOG_INFO, "%s", data);
}

void *thr_fn(void *arg)
{
	int signo;

	for (;;)
	{
		if (sigwait(&mask, &signo) != 0)
		{
			syslog(LOG_ERR, "sigwait, errno %d", errno);
			pthread_exit(0);
		}
		switch (signo)
		{
		case SIGHUP:
			syslog(LOG_INFO, "SIGHUP");
			reread();
			break;
		case SIGTERM:
			syslog(LOG_INFO, "SIGTERM; exit");
			pthread_exit(0);
		default:
			syslog(LOG_INFO, "signal: %d\n", signo);
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	time_t cur_time;
	int err;
	pthread_t tid;
	struct sigaction sa;
	char *cmd = "daemon";

	daemonize(cmd);

	if (already_running() != 0)
	{
		syslog(LOG_ERR, "already_running");
		exit(1);
	}

	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGHUP, &sa, NULL) == -1)
	{
		syslog(LOG_ERR, "%s: sigaction SIGHUP", cmd);
		exit(1);
	}

	if (sigfillset(&mask) == -1)
	{
		syslog(LOG_ERR, "sigfillset");
		exit(1);
	}

	if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) == -1)
	{
		syslog(LOG_ERR, "pthread_sigmask");
		exit(1);
	}

	if ((err = pthread_create(&tid, NULL, thr_fn, NULL)) == -1)
	{
		syslog(LOG_ERR, "pthread_create");
		exit(1);
	}

	for (int i = 0; i < 4; i++)
	{
		cur_time = time(NULL);
		syslog(LOG_NOTICE, "time: %s", ctime(&cur_time));
		sleep(3);
	}

	while (1)
	{
		sleep(10);
	}

	return 0;
}
