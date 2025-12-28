#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int childpid;
    if ((childpid = fork()) == -1)
    {
        perror("Can't fork.\n");
        return 1;
    }
    else if (childpid == 0)
    {
        printf("Child: %d\n", getpid());
        while (1)
            sleep(1);
        return 0;
    }
    else
    {
        printf("Parent: %d\n", getpid());
        while (1)
            sleep(1);
        return 0;
    }
}