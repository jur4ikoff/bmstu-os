#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define CHILDS_NUM 3

int main(void)
{
    int wait_status;
    pid_t child_pids[CHILDS_NUM], wait_child_pid;
    int sockets[2];
    char *strings[CHILDS_NUM + 1] = {"aaa", "bbbbbbbbbb", "ccccc", " "};
    char *prmsg = "parent";
    char buf[32];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
    {
        perror("socketpair");
        exit(1);
    }
    for (int i = 0; i < CHILDS_NUM; i++)
    {
        if ((child_pids[i] = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }
        else if (child_pids[i] == 0)
        {
            close(sockets[1]);
            if (write(sockets[0], strings[i], sizeof(strings[i])) == -1)
            {
                perror("write");
                exit(1);
            }
            printf("[child] %d sent %s\n", getpid(), strings[i]);
            read(sockets[0], buf, sizeof(buf));
            printf("[child] %d received %s\n", getpid(), buf);
            exit(0);
        }
        else
        {
            read(sockets[1], buf, sizeof(buf));
            printf("[parent] %d received %s\n", getpid(), buf);
            strcat(buf, "+ OK");
            if (write(sockets[1], buf, sizeof(buf)) == -1)
            {
                perror("write");
                exit(1);
            }
            printf("[parent] %d sent %s\n", getpid(), buf);
        }
    }
    close(sockets[1]);
    close(sockets[0]);
    for (int i = 0; i < CHILDS_NUM; i++)
    {
        wait_child_pid = wait(&wait_status);
        if (wait_child_pid == -1)
        {
            perror("Can't wait");
            exit(1);
        }
        if (WIFEXITED(wait_status))
        {
            printf("Child (PID: %d) exited with status = %d\n", wait_child_pid, WEXITSTATUS(wait_status));
        }
        else if (WIFSIGNALED(wait_status))
        {
            printf("Child (PID: %d) with (signal %d) killed\n", wait_child_pid, WTERMSIG(wait_status));
        }
        else if (WIFSTOPPED(wait_status))
        {
            printf("Child (PID: %d) with (signal %d) stopped\n", wait_child_pid, WSTOPSIG(wait_status));
        }
        else if (WIFCONTINUED(wait_status))
        {
            printf("Child (PID: %d) continued\n", wait_child_pid);
        }
    }
    return 0;
}