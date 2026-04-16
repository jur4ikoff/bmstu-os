#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>
#define SIZE 26
#define	LISTENQ	1024
#define SERV_PORT 9877
#define SB 0
#define SE 1
#define SF 2
#define P -1
#define V 1

struct sembuf start_produce[2] = {{SE, P, 0}, {SB, P, 0}};
struct sembuf stop_produce[2] = {{SB, V, 0}, {SF, V, 0}};
struct sembuf start_consume[2] = {{SF, P, 0}, {SB, P, 0}};
struct sembuf stop_consume[2] = {{SB, V, 0}, {SE, V, 0}};
char letter = 'a';
char *producer_ptr;
char *consumer_ptr;
struct thread_args {
    int semfd;
    int connfd;
    struct timespec start;
};

int flag = 1;

void handle_sigint(int sig) {
    flag = 0;
}
char storage[SIZE];

int semtimedop(int semfd, struct sembuf *sops, int nsops, int timeout_ms)
{
    struct sembuf local[nsops];
    memcpy(local, sops, nsops * sizeof(struct sembuf));
    for (int i = 0; i < nsops; i++)
        local[i].sem_flg |= IPC_NOWAIT;

    struct timespec deadline, now;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec  += timeout_ms / 1000;
    deadline.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    while (semop(semfd, local, nsops) == -1) {
        if (errno != EAGAIN)
            return -1;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec > deadline.tv_sec ||
           (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec)) {
            errno = EAGAIN;
            return -1;
        }
        usleep(1000);
    }
    return 0;
}

void* producer(void* arg) 
{
    struct thread_args *targ = (struct thread_args *) arg;
    struct timespec end;
    FILE *f;
    char buff[64];
    long long time;
    char send_char;
    if (semtimedop(targ->semfd, start_produce, 2, 10) == -1) {
        if (errno == EAGAIN) {
            write(targ->connfd, "buf full\n", 9);
            close(targ->connfd);
            free(targ);
            pthread_exit(NULL);
        }
        close(targ->connfd);
        free(targ);
        printf("Producer errno - %d, error - %s\n", errno, strerror(errno));
        pthread_exit(NULL);
    }
    *producer_ptr = letter;
    send_char = letter;
    if (letter == 'z') {
        letter -= 26;
        producer_ptr -= 26;
    }
    producer_ptr++;
    letter++;
    if (semop(targ->semfd, stop_produce, 2) == -1) {
        close(targ->connfd);
        free(targ);
        printf("Producer errno - %d, error - %s\n",  errno, strerror(errno));
        pthread_exit(NULL);
    }
    snprintf(buff, sizeof(buff), "produced: %c\n", send_char);
    if (write(targ->connfd, buff, sizeof(buff)) == -1)
    {
        perror("write");
        close(targ->connfd);
        free(targ);
        pthread_exit(NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (long long)(end.tv_sec - targ->start.tv_sec) * 1e9 + (end.tv_nsec - targ->start.tv_nsec);
    f = fopen("time.txt", "a");
    if (f == NULL)
    {
        perror("open");
        pthread_exit(NULL);
    }
    fprintf(f, "%lld\n", time);
    fclose(f);
    printf("p: %c\n", send_char);
    close(targ->connfd);
    free(targ);
    pthread_exit(NULL);
}

void* consumer(void* arg) 
{
    struct thread_args *targ = (struct thread_args *) arg;
    char buff[64];
    struct timespec end;
    char send_char;
    long long time;
    FILE *f;
    if (semtimedop(targ->semfd, start_consume, 2, 10) == -1) {
        if (errno == EAGAIN) {
            write(targ->connfd, "buf empty\n", 10);
            close(targ->connfd);
            free(targ);
            pthread_exit(NULL);
        }
        close(targ->connfd);
        free(targ);
        printf("Consumer errno - %d, error - %s\n", errno, strerror(errno));
        pthread_exit(NULL);
    }
    send_char = *consumer_ptr;
    if (*consumer_ptr == 'z')
        consumer_ptr -= 26;
    consumer_ptr++;
    if (semop(targ->semfd, stop_consume, 2) == -1) {
        printf("Consumer errno - %d, error - %s\n", errno, strerror(errno));
        pthread_exit(NULL);
    }
    snprintf(buff, sizeof(buff), "consumed: %c\n", send_char);
    if (write(targ->connfd, buff, sizeof(buff)) == -1)
    {
        perror("write");
        close(targ->connfd);
        free(targ);
        pthread_exit(NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (long long)(end.tv_sec - targ->start.tv_sec) * 1e9 + (end.tv_nsec - targ->start.tv_nsec);
    f = fopen("time.txt", "a");
    if (f == NULL)
    {
        perror("open");
        pthread_exit(NULL);
    }
    fprintf(f, "%lld\n", time);
    fclose(f);
    printf("c: %c\n", send_char);
    close(targ->connfd);
    free(targ);
    pthread_exit(NULL);
}


int main(void)
{
    struct sockaddr_in server_addr = {0};
    struct sockaddr_in client_addr;
    socklen_t client_len;
    ssize_t n;
    pthread_attr_t attr;
    pthread_t tid;
    int s;
    int listenfd;
    int connfd;
    int semfd;
    char buff[64];
    int num_of_char;
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    struct timespec start;
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0)
    {
        perror("socket");
        exit(-1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, LISTENQ) == -1)
    {
        perror("listen");
        exit(1);
    }

    semfd = semget(102, 3, IPC_CREAT | perms);
    if (semctl(semfd, SB, SETVAL, 1) == -1) {
        perror("semctl");
        exit(1);
    }

    if (semctl(semfd, SE, SETVAL, SIZE) == -1) {
        perror("semctl");
        exit(1);
    }

    if (semctl(semfd, SF, SETVAL, 0) == -1) {
        perror("semctl");
        exit(1);
    }

    producer_ptr = storage;
    consumer_ptr = storage;

    while (flag)
    {
        client_len = sizeof(client_addr);
        if ((connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len)) == -1)
        {
            if (errno == EINTR)
            {
                printf("Accept errno - %d, error - %s\n", errno, strerror(errno));
                continue;
            }

            perror("accept");
            break;
        }
        clock_gettime(CLOCK_MONOTONIC, &start);
        n = read(connfd, buff, sizeof(buff));
        if (n == -1)
        {
            printf("Read errno - %d, error - %s\n", errno, strerror(errno));
            perror("read");
            exit(1);
        }
        pthread_attr_init(&attr);
        s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        struct thread_args *targs = malloc(sizeof(struct thread_args));
        targs->semfd = semfd;
        targs->connfd = connfd;
        targs->start = start;
        if (s != 0) 
        {
            perror("pthread_attr_setdetachstate");
            exit(1);
        }
        if (buff[0] == 'c')
        {
            if (pthread_create(&tid, &attr, consumer, targs) != 0) {
                perror("pthread_create");
        }
        } else 
        {
            if (pthread_create(&tid, &attr, producer, targs) != 0) {
                perror("pthread_create");
            }
        }
    }
    close(listenfd);
    if (semctl(semfd, 0, IPC_RMID, NULL) == -1) {
        perror("semctl");
        exit(1);
    }
    return 0;
}


