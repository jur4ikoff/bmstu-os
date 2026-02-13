#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

bool flag = 0;

void sighandler(int signum)
{
    flag = 1;
    printf("pid: %d signum: %d\n", getpid(), signum);

}

int main(void)
{
      pid_t childpids[2];
    int pipefds[2];
    const char *strings[3] = {"aaaa", "bbbbbbbbbbbbbbbb"};
   // printf("Parent: (PID: %d)\n", getpid());
    if (signal(SIGINT, sighandler) == SIG_ERR)
    {
        perror("Cant register signal handler!");
        exit(1);
    }
    sleep(2);
  
    if (pipe(pipefds) == -1)
    {
        perror("Cant create a pipe!");
        exit(1);
    }

    for (size_t i = 0; i < 2;i++ )
    {
        childpids[i] = fork();
        if (childpids[i] == -1)
        {
            perror("Cant fork!");
            exit(1);
        }
        if (childpids[i] == 0)
        {
            if (flag == 1)
            {
                if (close(pipefds[0]) == -1)
                {
                    perror("Cant close");
                    exit(1);
                }
                if (write(pipefds[1], strings[i], strlen(strings[i])) == -1)
                {
                    perror("Cant write");
                    exit(1);
                }
            }
            exit(0);
        }
    }
    for (size_t i = 0; i < 2; ++i)
    {
        int wstatus;
        pid_t w = wait(&wstatus);
        if (w == -1)
        {
            perror("Cant wait!\n");
            exit(1);
        }
       // printf("Child #%zu (PID: %d) exited.\n", i, w);
        if (WIFEXITED(wstatus))
            printf("Child %d exited. Code: %d\n", w, WEXITSTATUS(wstatus));
        if (WIFSIGNALED(wstatus))
            printf("Child #%d terminated. Signal #: %d\n", w, WTERMSIG(wstatus));
        if (WIFSTOPPED(wstatus))
            printf("Child #%d stopped. Signal #: %d\n", w, WSTOPSIG(wstatus));
       // puts("");
    }
    if (close(pipefds[1]) == -1)
        perror("Cant close");

    char buf[128];
    for (int i = 0; i < 2; i++)
    {
        memset(buf, 0, sizeof(buf));
        ssize_t s;
        if ((s = read(pipefds[0], buf, strlen(strings[i]))) == -1)
            perror("Cant read");
        printf("%ld %s\n", s, buf);
    }
    return 0;
}
