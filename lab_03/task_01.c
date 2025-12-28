#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
            printf("CHILD process #%zu: PID: %d, PPID: %d, PGRP: %d\n", i, getpid(), getppid(), getpgrp());
            sleep(2);
            printf("CHILD process #%zu: PID: %d, PPID: %d, PGRP: %d\n", i, getpid(), getppid(), getpgrp());
            exit(0);
        }
    }
    printf("PARENT process: PID: %d, PPID: %d, PGRP: %d\nchild[0]: %d, child[1]: %d\n", getpid(), getppid(), getpgrp(), childpids[0], childpids[1]);
    sleep(5);
    return 0;
}
