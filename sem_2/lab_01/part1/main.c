#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define CHILDS 3
#define BUF_SIZE 1024

int main() {

    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        printf("errno: %d", errno);
        perror("socketpair");
        exit(1);
    }
    
    pid_t childs[CHILDS];
    const char* messages[] = {"aaaaa","bb","ccc"};
    char buf[BUF_SIZE];

    for (int i = 0; i < CHILDS; i++) {
        if ((childs[i] = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        else if (childs[i] == 0) {
            if (close(sockets[1]) == -1) {
                printf("errno: %d", errno);
                perror("close");
                exit(1);
            };
            if (write(sockets[0], messages[i], strlen(messages[i]) + 1) == -1) {
                printf("errno: %d", errno);
                perror("write");
                exit(1);
            } 
            printf("child %d write %s\n", getpid(), messages[i]);
            if (read(sockets[0], buf, sizeof(buf)) == -1) {
                printf("errno: %d", errno);
                perror("read");
                exit(1);
            }
            printf("child %d read: %s\n", getpid(), buf);
            
            if (close(sockets[0]) == -1) {
                printf("errno: %d\n", errno);
                perror("close");
                exit(1);
            };
            exit(0);
        }
        else {
            if (read(sockets[1], buf, sizeof(buf)) == -1) {
                printf("errno: %d\n", errno);
                perror("read");
                exit(1);
            }            
            printf("parent %d read: %s \n", getpid(), buf);
            strcat(buf, "ok");
            if (write(sockets[1], buf, sizeof(buf)) == -1) {
                printf("errno: %d\n", errno);
                perror("write");
                exit(1);
            }             
            printf("parent %d write %s\n", getpid(), buf);
        }
    }

    if (close(sockets[0]) == -1) {
        printf("errno: %d\n", errno);
        perror("close");
        exit(1);
    }
    if (close(sockets[1]) == -1) {
        printf("errno: %d\n", errno);
        perror("close");
        exit(1);
    }

    for (size_t i = 0; i < CHILDS; ++i)
    {
        int status;
        pid_t childpid = wait(&status);
        if (childpid == -1)
        {
            perror("Can`t wait!\n");
            exit(1);
        }
        printf("Child #%zu (PID: %d) exited.\n", i, childpid);
        if (WIFEXITED(status))
            printf("Child #%zu exited. Code: %d\n", i, WEXITSTATUS(status));
        if (WIFSIGNALED(status))
            printf("Child #%zu terminated. Signal #: %d\n", i, WTERMSIG(status));
        if (WIFSTOPPED(status))
            printf("Child #%zu stopped. Signal #: %d\n", i, WSTOPSIG(status));
        puts("");
    }
}