#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define AMOUNT_OF_CHILDS 3

void client_worker(int id) {
    char buf[64];
    char msg[32];
    struct sockaddr server_addr, client_addr;

    int sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        printf("%d", errno);
        exit(1);
    }
    server_addr.sa_family = AF_UNIX;
    strcpy(server_addr.sa_data, "myserver.s");

    client_addr.sa_family = AF_UNIX;
    sprintf(client_addr.sa_data, "%d.s", getpid());
    
    unlink(client_addr.sa_data);
    socklen_t server_addr_len = sizeof(server_addr);
    
    if (bind(sock_fd, &client_addr, sizeof(client_addr)) == -1) {
        perror("bind");
        printf("%d", errno);
        exit(1);
    }
    
    sprintf(msg, "%d", getpid());    
    if (sendto(sock_fd, msg, strlen(msg), 0, &server_addr, server_addr_len) == -1) {
        perror("sendto");
        exit(1);
    }

    int bytes_read = recvfrom(sock_fd, buf, sizeof(buf) - 1, 0, &server_addr, &server_addr_len);
    if (bytes_read == -1) {
        perror("recvfrom");
        printf("%d", errno);
        exit(1);
    }
    buf[bytes_read] = '\0';

    printf("[Child %d] Received from server: %s\n", id, buf);
    sleep(3);
    close(sock_fd);
    unlink(client_addr.sa_data);
    exit(0);
}

int main() {
    pid_t childpids[AMOUNT_OF_CHILDS];

    for (size_t i = 0; i < AMOUNT_OF_CHILDS; ++i) 
    {
        if ((childpids[i] = fork()) == -1) 
        {
            perror("fork");
            printf("%d", errno);
            exit(1);
        }

        if (childpids[i] == 0) {
            client_worker(i);
        }
    }

    for (size_t i = 0; i < AMOUNT_OF_CHILDS; ++i)
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

    return 0;
}