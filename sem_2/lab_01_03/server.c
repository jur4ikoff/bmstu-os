#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

struct client_args {
    int connfd;
    int semfd;
    char *letter;
    char *buffer;
    int *prod_ind;
    int *cons_ind;
    FILE *logfile;
};

#define SERV_PORT 9878
#define BUF_LEN 50
#define SB 0
#define SE 1
#define SF 2
#define P -1
#define V  1
struct sembuf start_produce[2] = { {SE, P, 0}, {SB, P, 0} };
struct sembuf stop_produce[2] =  { {SB, V, 0}, {SF, V, 0} };
struct sembuf start_consume[2] = { {SF, P, 0}, {SB, P, 0} };
struct sembuf stop_consume[2] =  { {SB, V, 0}, {SE, V, 0} };
bool fl = 1;

void producer(int connfd, char *buffer, char *letter, int *prod_ind, int semfd) {
    char buf[1];
    if (semop(semfd, start_produce, 2) == -1) {
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
    if (semop(semfd, stop_produce, 2) == -1) {
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
    if (semop(semfd, start_consume, 2) == -1) {
        perror("semop");
        exit(1);
    }
    buf[0] = buffer[*cons_ind];
    if (*cons_ind == BUF_LEN - 1)
        *cons_ind = 0;
    else
        (*cons_ind)++;
    if (semop(semfd, stop_consume, 2) == -1) {
        perror("semop");
        exit(1);
    }
    printf("Consume and send: %c\n", buf[0]);
    if (send(connfd, &buf, sizeof(buf), 0) == -1) {
        perror("send");
        exit(1);
    }
}

void* process_client(void *args) {
    char buf[10];
    struct timespec start, end;
    struct client_args *client_args = (struct client_args *)args;
    int connfd = client_args->connfd;
    char *buffer = client_args->buffer;
    char *letter = client_args->letter;
    int *prod_ind = client_args->prod_ind;
    int *cons_ind = client_args->cons_ind;
    FILE *logfile = client_args->logfile;
    int semfd = client_args->semfd;

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
    pthread_t th;
    struct sigaction sa;
    struct client_args *args;
    char letter = 'a';
    char buffer[BUF_LEN];
    int prod_ind = 0;
    int cons_ind = 0;
    FILE *logfile;

    logfile = fopen("server.log", "w");
    if (logfile == NULL) {
        perror("fopen");
        exit(1);
    }
    if ((semkey = ftok("./key.txt", 1)) == -1) {
        perror("ftok");
        exit(1);
    }
    if ((semfd = semget(semkey, 3, IPC_CREAT | perms)) == -1) {
        perror("semget");
        exit(1);
    }
    if (semctl(semfd, SB, SETVAL, 1) == -1) {
        perror("semctl");
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
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
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
    if (listen(listenfd, 1024) == -1) {
        perror("listen");
        exit(1);
    }
    while (fl) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd == -1) {
            printf("errno %d\n", errno);
            if (errno != EINTR) {
                perror("accept");
                exit(1);
            }
        } else {
            args = malloc(sizeof(struct client_args));
            if (args == NULL) {
                perror("malloc");
                exit(1);
            }
            args->connfd = connfd;
            args->semfd = semfd;
            args->buffer = buffer;
            args->letter = &letter;
            args->prod_ind = &prod_ind;
            args->cons_ind = &cons_ind;
            args->logfile = logfile;
            if (pthread_create(&th, NULL, process_client, args) == -1) {
                perror("pthread_create");
                exit(1);
            }
            if (pthread_detach(th) == -1) {
                perror("pthread_detach");
                exit(1);
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
