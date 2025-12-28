#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define AMOUNT_OF_CHILDS 2

int main()
{
    int childpid[AMOUNT_OF_CHILDS];

    for (int i = 0; i < AMOUNT_OF_CHILDS; i++)
    {
        if ((childpid[i] = fork()) < 0)
        {
            printf("Can't fork\n");
            exit(1);
        }
        if (childpid[i] == 0)
        {
            printf("[child %d] before sleep: pid=%d, ppid=%d, group=%d\n",
                   i, getpid(), getppid(), getpgrp());
            sleep(3);
            printf("[child %d] after sleep:  pid=%d, ppid=%d, group=%d\n",
                   i, getpid(), getppid(), getpgrp());
            // exit(0);
        }
        if (childpid[i] > 0)
        {
            printf("Parent process: pid=%d, group=%d, child[0] = %d, child[1] = %d\n",
                   getpid(),
                   getpgrp(), childpid[0], childpid[1]);
        }
    }
    return 0;
}