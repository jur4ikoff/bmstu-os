#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENTS 10
#define SERV_PORT 9878
#define BUF_LEN 50
#define SE 0
#define SF 1
#define P -1
#define V  1
struct sembuf start_produce[1] = { {SE, P, 0} };
struct sembuf stop_produce[1] =  { {SF, V, 0} };
struct sembuf start_consume[1] = { {SF, P, 0} };
struct sembuf stop_consume[1] =  { {SE, V, 0} };
struct timespec timeout = {.tv_sec = 0, .tv_nsec = 500000};
bool fl = 1;

void setnonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl");
        exit(1);
    }
}

void producer(int connfd, char *buffer, char *letter, int *prod_ind, int semfd) {
    char buf[1];
    if (semtimedop(semfd, start_produce, 1, &timeout) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            buf[0] = '!';
            printf("send error\n");
            if (send(connfd, &buf, sizeof(buf), 0) == -1) {
                perror("send");
                exit(1);
            }
            return;
        }
        perror("semop");
        exit(1);
    }
    buffer[*prod_ind] = *letter;
    printf("Produce: %c ", *letter);
    if (*letter == 'z')
        *letter = 'a';
    else
        (*letter)++;
    if (*prod_ind == BUF_LEN - 1)
        *prod_ind = 0;
    else
        (*prod_ind)++;
    if (semop(semfd, stop_produce, 1) == -1) {
        perror("semop");
        exit(1);
    }
    buf[0] = 'o';
    printf("send: %c\n", buf[0]);
    if (send(connfd, &buf, sizeof(buf), 0) == -1) {
        perror("send");
        exit(1);
    }
}

void consumer(int connfd, char *buffer, char *letter, int *cons_ind, int semfd) {
    char buf[1];
    if (semtimedop(semfd, start_consume, 1, &timeout) == -1) {
        if (errno == EAGAIN) {
            buf[0] = '!';
            printf("send error\n");
            if (send(connfd, &buf, sizeof(buf), 0) == -1) {
                perror("send");
                exit(1);
            }
            return;
        }
        perror("semop");
        exit(1);
    }
    buf[0] = buffer[*cons_ind];
    if (*cons_ind == BUF_LEN - 1)
        *cons_ind = 0;
    else
        (*cons_ind)++;
    if (semop(semfd, stop_consume, 1) == -1) {
        perror("semop");
        exit(1);
    }
    printf("Consume and send: %c\n", buf[0]);
    if (send(connfd, &buf, sizeof(buf), 0) == -1) {
        perror("send");
        exit(1);
    }
}

void* process_client(int connfd, char *buffer, char *letter, int *prod_ind, int *cons_ind, FILE *logfile, int semfd) {
    char buf[10];
    struct timespec start, end;
    printf("Connected\n");
    clock_gettime(CLOCK_REALTIME, &start);
    if (recv(connfd, buf, sizeof(buf), 0) == -1) {
        perror("recv");
        exit(1);
    }
    if (buf[0] == 'p')
        producer(connfd, buffer, letter, prod_ind, semfd);
    else if (buf[0] == 'c')
        consumer(connfd, buffer, letter, cons_ind, semfd);
    else
        printf("Unknown request: %c\n", buf[0]);
    close(connfd);
    clock_gettime(CLOCK_REALTIME, &end);
    fprintf(logfile, "%ld\n", end.tv_nsec - start.tv_nsec);
    return NULL;
}

void sigint_handler(int signum) {
    fl = 0;
}

int main(void) {
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    key_t semkey;
    int semfd;
    int connfd, listenfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr = { 0 };
    FILE *logfile;
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;
    char letter = 'a';
    char buffer[BUF_LEN];
    int prod_ind = 0;
    int cons_ind = 0;

    logfile = fopen("epoll_server.log", "w");
    if (logfile == NULL) {
        perror("fopen");
        exit(1);
    }
    semkey = ftok("./key.txt", 1);
    if (semkey == -1) {
        perror("ftok");
        exit(1);
    }
    semfd = semget(semkey, 2, IPC_CREAT | perms);
    if (semfd == -1) {
        perror("semget");
        exit(1);
    }
    if (semctl(semfd, SE, SETVAL, BUF_LEN) == -1) {
        perror("semctl");
        exit(1);
    }
    if (semctl(semfd, SF, SETVAL, 0) == -1) {
        perror("semctl");
        exit(1);
    }
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        exit(1);
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(listenfd, 10) == -1) {
        perror("listen");
        exit(1);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(1);
    }
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl");
        exit(1);
    }
    while (fl) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(1);
        }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listenfd) {
                connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
                if (connfd == -1) {
                    perror("accept");
                    exit(1);
                }
                setnonblocking(connfd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
                    perror("epoll_ctl");
                    exit(1);
                }
            } else {
                process_client(events[n].data.fd, buffer, &letter, &prod_ind, &cons_ind, logfile, semfd);
            }
        }
    }
    close(listenfd);
    if (semctl(semfd, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(1);
    }
    fclose(logfile);
    exit(0);
}
