#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    int pipefd[2];
    pid_t childPIDS[2];
    char *const message[2] = {
        "aaa", "bbbbbbbbbb"};
    if (pipe(pipefd) == -1)
    {
        printf("Can`t pipe!\n");
        exit(1);
    }
    for (int i = 0; i < 2; i++)
    {
        if ((childPIDS[i] = fork()) == -1)
        {
            exit(1);
        }
        else if (childPIDS[i] == 0)
        {
            close(pipefd[0]);
            write(pipefd[1], message[i], strlen(message[i]));
            exit(0);
        }
    }

    for (int i = 0; i < 2; i++)
    {
        int wstatus;
        pid_t w = wait(&wstatus);
        if (w == -1)
        {
            exit(1);
        }
        if (WIFEXITED(wstatus))
            printf("Child exited with code %d\n", WEXITSTATUS(wstatus));
        else if (WIFSIGNALED(wstatus))
            printf("Child terminated, recieved signal %d\n", WTERMSIG(wstatus));
        else if (WIFSTOPPED(wstatus))
            printf("Child stopped, recieved signal %d\n", WSTOPSIG(wstatus));
    }

    for (int i = 0; i < 2; i++)
    {
        char buffer[30];
        close(pipefd[1]);
        ssize_t number_written;
        if (i == 0)
        {
            number_written = read(pipefd[0], buffer, 3);
            buffer[3] = '\0';
        }
        if (i == 1)
        {
            number_written = read(pipefd[0], buffer, 10);
            buffer[10] = '\0';
        }
        if (i > 1)
        {
            number_written = read(pipefd[0], buffer, 1);
        }
        else
        {
            printf("%s\n", buffer);
        }
        printf("-----\n");
    }

    close(pipefd[0]);
    return 0;
}