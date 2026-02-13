#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const char *programs[2] = {"./demo/prg_01", "./demo/prg_02"};

int main(void)
{
    pid_t childpids[2];
    for (size_t i = 0; i < 2; ++i)
    {
        childpids[i] = fork();
        if (childpids[i] <= -1)
        {
            perror("Could not fork!");
            exit(1);
        }
        if (childpids[i] == 0)
        {
            printf("CHILD %d\n", getpid());
            if (execlp(programs[i], programs[i], NULL) == -1)
            {
                perror("Could not exec!");
                exit(1);
            }
            // printf("CHILD %d\n", getpid());
        }
        else
        {
            printf("PARENT %d\n", getpid());
        }
    }
    for (size_t i = 0; i < 2; ++i)
    {
        int wstatus;
        pid_t w = waitpid(childpids[i], &wstatus, 0);
        if (w == -1)
        {
            perror("Unable wait!\n");
            exit(1);
        }
        printf("Child #%zu (PID: %d) exited.\n", i, w);
        if (WIFEXITED(wstatus))
            printf("Child #%d exited. Code: %d\n", w, WEXITSTATUS(wstatus));
        if (WIFSIGNALED(wstatus))
            printf("Child #%d terminated. Signal #: %d\n", w, WTERMSIG(wstatus));
        if (WIFSTOPPED(wstatus))
            printf("Child #%d stopped. Signal #: %d\n", w, WSTOPSIG(wstatus));
        puts("");
    }
    printf("Parent process: PID: %d, PPID: %d, PGRP: %d\nchild[0]: %d, child[1]: %d\n", getpid(), getppid(), getpgrp(), childpids[0], childpids[1]);
    return 0;
}
