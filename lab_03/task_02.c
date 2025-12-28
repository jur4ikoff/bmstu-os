#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t childpids[2];
    for (size_t i = 0; i < 2; ++i)
    {
        childpids[i] = fork();
        if (childpids[i] == -1)
        {
            perror("Could not fork!");
            exit(1);
        }
        if (childpids[i] == 0)
        {
            printf("Child #%zu: PID: %d, PPID: %d, PGRP: %d\n", i, getpid(), getppid(), getpgrp());
            if (i == 0)
                sleep(10);
            exit(0);
        }
        else
        {
            int status;
            pid_t childpid = wait(&status);
            if (childpid <= -1)
            {
                perror("Unable wait!\n");
                exit(1);
            }
            printf("Child #%zu (PID: %d) exited.\n", i, childpid);
            if (WIFEXITED(status))
                printf("Child #%zu exited. Code: %d\n", i, WEXITSTATUS(status));
            if (WIFSIGNALED(status))
                printf("Child #%zu terminated. Signal #: %d\n", i, WTERMSIG(status));
            if (WIFSTOPPED(status))
                printf("Child #%zu stopped. Signal #: %d\n", i, WSTOPSIG(status));
        }
    }
    printf("Parent process: PID: %d, PPID: %d, PGRP: %d\nchild[0]: %d, child[1]: %d\n", getpid(), getppid(), getpgrp(), childpids[0], childpids[1]);
    return 0;
}
