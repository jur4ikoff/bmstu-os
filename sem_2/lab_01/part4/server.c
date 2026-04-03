#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#define PORT 8080
#define N 4096
#define SB 0
#define SE 1
#define SF 2
#define P -1
#define V  1

struct sembuf start_produce[2] = { {SE, P, 0}, {SB, P, 0} };
struct sembuf stop_produce[2] =  { {SB, V, 0}, {SF, V, 0} };
struct sembuf start_consume[2] = { {SF, P, 0}, {SB, P, 0} };
struct sembuf stop_consume[2] =  { {SB, V, 0}, {SE, V, 0} };

int semfd;
char buffer[N];
int in = 0, out = 0;
char global_letter = 'A';
FILE *log_fp;
int flag = 1;

void sig_handler(int signum) {
    flag = 0;
    printf("Catch sigint\n");
}

void* produce(void* arg) {
    struct timespec start, end;
    int* fd_ptr = (int*)arg;
    int fd = *fd_ptr;
    free(fd_ptr);
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (semop(semfd, start_produce, 2) == -1) {
        perror("semop\n");
        printf("errno : %d\n", errno);
        return NULL;
    }

    buffer[in] = global_letter;
    printf("[Producer] Put '%c' at index %d\n", global_letter, in);
    in = (in + 1) % N;
    if (global_letter == 'Z') 
        global_letter = 'A'; 
    else 
        global_letter++;
    
    if (semop(semfd, stop_produce, 2) == -1) {
        perror("semop\n");
        printf("errno : %d\n", errno);
        exit(1);
    }
    
    if (send(fd, "OK", 2, 0) < 0) {
        perror("send");
        return NULL;
    }
    close(fd);
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    fprintf(log_fp, "%ld\n", elapsed);
    fflush(log_fp);
    return NULL;
}

void* consume(void* arg) {
    struct timespec start, end;
    int* fd_ptr = (int*)arg;
    int fd = *fd_ptr;
    free(fd_ptr);
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (semop(semfd, start_consume, 2) == -1) {
        perror("semop\n");
        printf("errno : %d\n", errno);
        return NULL;
    }

    char item = buffer[out];
    printf("[Consumer] Got '%c' from index %d\n", item, out);
    out = (out + 1) % N;
    
    if (semop(semfd, stop_consume, 2) == -1) {
        perror("semop\n");
        printf("errno : %d\n", errno);
        return NULL;
    }

    if (send(fd, &item, 1, 0) < 0) {
        perror("send");
        return NULL;
    }
    close(fd);
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    fprintf(log_fp, "%lld\n", elapsed);
    fflush(log_fp);
    return NULL;
}


int main() {
    struct sockaddr_in server_addr;
    int listenfd;
    key_t semkey;
    pthread_attr_t attr;
    struct sigaction sa;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    log_fp = fopen("measures.txt", "w");
    if (!log_fp) {
        perror("fopen");
        exit(1);
    }

    if ((semkey = ftok("./ipc_key", 1)) == -1) {
        perror("ftok"); 
        exit(1); 
    }
    if ((semfd = semget(semkey, 3, IPC_CREAT | perms)) == -1) { 
        perror("semget"); 
        exit(1); 
    }
    if (semctl(semfd, SB, SETVAL, 1) == -1) {
        perror("semctl\n");
        exit(1);
    }
    if (semctl(semfd, SE, SETVAL, N) == -1) {
        perror("semctl\n");
        exit(1);
    }
    if (semctl(semfd, SF, SETVAL, 0) == -1) {
        perror("semctl\n");
        exit(1);
    }
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 50) < 0) {
        perror("listen");
        exit(1);
    }
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    while (flag) {
        struct sockaddr_in client_addr;
        socklen_t clilen = sizeof(client_addr);
        int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &clilen);
        if (connfd < 0) {
            perror("accept");
            if (errno == EINTR) 
                exit(1); 
        }
        char type;
        if (recv(connfd, &type, 1, 0) <= 0) {
            close(connfd);
            exit(1);
        }

        int* arg_fd = malloc(sizeof(int));
        if (arg_fd == NULL) {
            close(connfd);
            perror("malloc");
            exit(1);
        }
        *arg_fd = connfd;
        pthread_t tid;
        if (type == 'P') {
            pthread_create(&tid, &attr, produce, arg_fd);
        } else if (type == 'C') {
            pthread_create(&tid, &attr, consume, arg_fd);
        } else {
            free(arg_fd);
            close(connfd);
        }    
    }

    close(listenfd);
    if (fclose(log_fp) != 0) {
        perror("fclose");
        exit(1);
    }
    if (semctl(semfd, 0, IPC_RMID) < 0) {
        perror("semctl");
        exit(1);
    } 
    return 0;
}