#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define SOCK_NAME "myserver.s"
int sock_fd;
int fl = 1;

void sigint_handler(int signum) {
    close(sock_fd);
    unlink(SOCK_NAME);
    fl = 0;
}

int main() {
    struct sockaddr server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buf[64];
    int bytes_read;
    struct timespec ts;

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        exit(1);
    }
    server_addr.sa_family = AF_UNIX;
    strcpy(server_addr.sa_data, SOCK_NAME);
    if (bind(sock_fd, &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }
    while (fl) {
        bytes_read = recvfrom(sock_fd, buf, sizeof(buf), 0, &client_addr, &client_addr_len);
        if (bytes_read == -1) {
            if (!fl) break;
            perror("recvfrom");
            exit(1);
        }
        buf[bytes_read] = '\0';
        printf("recieve: %s\n", buf);

        clock_gettime(CLOCK_REALTIME, &ts);
        sprintf(buf, "%lld%09ld", (long long)ts.tv_sec, ts.tv_nsec);

        printf("send: %s\n", buf);
        if (sendto(sock_fd, buf, strlen(buf), 0, &client_addr, client_addr_len) == -1) {
            perror("sendto");
            exit(1);
        }
    }
    return 0;
}