#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ITERATIONS  1000
// #define SERVER_IP   "10.125.121.32"
// #define SERVER_IP   "10.125.121.11"
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 9878
#define MSG_SIZE    8

#define OP_PRODUCER 'P'
#define OP_CONSUMER 'C'

static int loop_flag = 1;

void sig_handler(int sig)
{
    loop_flag = 0;
}

int connect_to_server(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }
    return fd;
}

int producer(int fd, char* letter)
{
    char response[MSG_SIZE] = {0};
    if (recv(fd, response, MSG_SIZE, 0) <= 0) {
        perror("recv");
        return -1;
    }
    if (strcmp(response, "WAIT") == 0) {
        printf("P: buffer full\n");
        return 1;
    }
    printf("P: '%c' -> %s\n", *letter, response);
    *letter = (*letter == 'z') ? 'a' : *letter + 1;
    return 0;
}

int consumer(int fd)
{
    char response[MSG_SIZE] = {0};
    if (recv(fd, response, MSG_SIZE, 0) <= 0) {
        perror("recv");
        return -1;
    }
    if (strcmp(response, "WAIT") == 0) {
        printf("C: buffer empty\n");
        return 1;
    }
    printf("C: got '%c'\n", response[0]);
    return 0;
}

int main(void)
{
    signal(SIGINT, sig_handler);
    char letter = 'a';
    for (int i = 0; i < ITERATIONS && loop_flag; i++) {
        char op = ((i % 3) == 2) ? OP_CONSUMER : OP_PRODUCER;
        int fd = connect_to_server();
        if (fd == -1) {
            return EXIT_FAILURE;
        }
        int rc;
        if (op == OP_PRODUCER) {
            char msg[2] = {op, letter};
            if (send(fd, msg, 2, 0) == -1) {
                perror("send");
                close(fd);
                return EXIT_FAILURE;
            }
            rc = producer(fd, &letter);
        } else {
            if (send(fd, &op, 1, 0) == -1) {
                perror("send");
                close(fd);
                return EXIT_FAILURE;
            }
            rc = consumer(fd);
        }
        close(fd);
        if (rc == -1) {
            return EXIT_FAILURE;
        }
    }
    printf("Done.\n");
    return EXIT_SUCCESS;
}
