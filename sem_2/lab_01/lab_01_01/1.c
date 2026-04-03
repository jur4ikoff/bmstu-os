#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#define N 3
int main(void)
{
    pid_t child_pid[N];
    const char *messages[] = {"aaa", "bbbbbbbb", "c"};
    int sockfd[2];
    char buf[64];
    char buf_rcv[64];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == -1)
    {
        perror("socketpair");
        printf("%d\n", errno);
        exit(1);
    }
    for (size_t i = 0; i < N; i++)
    {
        if ((child_pid[i] = fork()) == -1)
        {
            perror("fork");
            printf("%d\n", errno);
            exit(1);
        }
        else if (child_pid[i] == 0)
        {
            if (close(sockfd[1]) == -1)
            {
                perror("close");
                printf("%d\n", errno);
                exit(1);
            }
            if (write(sockfd[0], messages[i], strlen(messages[i]) + 1) == -1)
            {
                perror("write");
                printf("%d\n", errno);
                exit(1);
            }
            printf("[child] %d sent %s\n", getpid(), messages[i]);
            if (read(sockfd[0], buf, sizeof(buf)) == -1)
            {
                perror("read");
                printf("%d\n", errno);
                exit(1);
            }
            printf("[child] %d received %s\n", getpid(), buf);
            if (close(sockfd[0]) == -1)
            {
                perror("close");
                printf("%d\n", errno);
                exit(1);
            }
            exit(0);
        }
        else
        {
            if (read(sockfd[1], buf, sizeof(buf)) == -1)
            {
                perror("read");
                printf("%d\n", errno);
                exit(1);
            }
            printf("[parent] %d received %s\n", getpid(), buf);
            sprintf(buf_rcv, "%sok", buf);
            if (write(sockfd[1], buf_rcv, sizeof(buf_rcv)) == -1)
            {
                perror("write");
                printf("%d\n", errno);
                exit(1);
            }
            printf("[parent] %d sent %s\n", getpid(), buf_rcv);
        }
    }
    if (close(sockfd[0]) == -1)
    {
        perror("close");
        printf("%d\n", errno);
        exit(1);
    }
    if (close(sockfd[1]) == -1)
    {
        perror("close");
        printf("%d\n", errno);
        exit(1);
    }
    for (size_t i = 0; i < N; i++)
    {
        int status;
        waitpid(child_pid[i], &status, 0);
        if (WIFEXITED(status))
            printf("Child (pid: %d) exited with code %d\n", child_pid[i], WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Child (pid: %d) received signal %d\n", child_pid[i], WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("Child (pid: %d) received signal %d\n", child_pid[i], WSTOPSIG(status));
    }
    exit(0);
}