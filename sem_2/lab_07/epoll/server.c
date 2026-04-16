#define _GNU_SOURCE

#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define SERVER_PORT       9878
#define SERVER_BACKLOG    5
#define BUF_SIZE          52
#define MAX_EVENTS        64
#define TOTAL_CONNECTIONS 900

#define OP_PRODUCER 'P'
#define OP_CONSUMER 'C'

#define SEM_KEY           200
#define SEM_STORAGE_EMPTY 0
#define SEM_STORAGE_FULL  1
#define SEM_LOCK          -1
#define SEM_UNLOCK        1

static char ring_buffer[BUF_SIZE];
static int write_idx = 0;
static int read_idx = 0;

static int semaphore_id;
static int listen_fd;
static int epoll_fd;
static int should_exit = 0;

#define LOG_FILE "select_server.txt"
static FILE* log_file = NULL;

struct sembuf stop_write = {SEM_STORAGE_EMPTY, SEM_LOCK, IPC_NOWAIT | SEM_UNDO};
struct sembuf start_read = {SEM_STORAGE_FULL, SEM_UNLOCK, SEM_UNDO};
struct sembuf stop_read = {SEM_STORAGE_FULL, SEM_LOCK, IPC_NOWAIT | SEM_UNDO};
struct sembuf start_write = {SEM_STORAGE_EMPTY, SEM_UNLOCK, SEM_UNDO};

void sig_handler(int sig)
{
    printf("\nServer shutting down...\n");
    shutdown(listen_fd, SHUT_RDWR);
    close(listen_fd);
    should_exit = 1;
}

void producer(int fd)
{
    char letter;
    if (recv(fd, &letter, 1, 0) <= 0) {
        return;
    }
    if (semop(semaphore_id, &stop_write, 1) == -1) {
        send(fd, "WAIT", 5, 0);
        printf("[P %d] FULL, rejected '%c'\n", fd, letter);
        return;
    }
    ring_buffer[write_idx] = letter;
    write_idx = (write_idx + 1) % BUF_SIZE;
    semop(semaphore_id, &start_read, 1);
    send(fd, "OK", 3, 0);
    printf("[P %d] put '%c'\n", fd, letter);
}

void consumer(int fd)
{
    if (semop(semaphore_id, &stop_read, 1) == -1) {
        send(fd, "WAIT", 5, 0);
        printf("[C %d] EMPTY\n", fd);
        return;
    }
    char resp[2];
    resp[0] = ring_buffer[read_idx];
    resp[1] = '\0';
    read_idx = (read_idx + 1) % BUF_SIZE;
    semop(semaphore_id, &start_write, 1);
    send(fd, resp, 2, 0);
    printf("[C %d] got '%c'\n", fd, resp[0]);
}

void client_service(int fd, struct timespec* time_start)
{
    char op;
    if (recv(fd, &op, 1, 0) <= 0) {
        close(fd);
        return;
    }
    if (op == OP_PRODUCER) {
        producer(fd);
    } else if (op == OP_CONSUMER) {
        consumer(fd);
    }
    struct timespec time_end;
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    long elapsed_ns = (time_end.tv_sec - time_start->tv_sec) * 1000000000L + (time_end.tv_nsec - time_start->tv_nsec);
    fprintf(log_file, "%ld\n", elapsed_ns);
    close(fd);
}

int main(void)
{
    struct sockaddr_in server_addr = {0}, client_addr;
    socklen_t client_len = sizeof(client_addr);
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, SERVER_BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, sig_handler);
    int permissions = S_IRWXU | S_IRWXG | S_IRWXO;
    semaphore_id = semget(SEM_KEY, 2, IPC_CREAT | permissions);
    if (semaphore_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    if (semctl(semaphore_id, SEM_STORAGE_EMPTY, SETVAL, BUF_SIZE) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    if (semctl(semaphore_id, SEM_STORAGE_FULL, SETVAL, 0) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
    // int served = 0;
    printf("server started on port %d (buffer: %d, epoll)\n", SERVER_PORT, BUF_SIZE);
    struct epoll_event events[MAX_EVENTS];
    while (!should_exit) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }
        struct timespec time_start;
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listen_fd) {
                int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                if (conn_fd == -1) {
                    perror("accept");
                    continue;
                }
                ev.events = EPOLLIN;
                ev.data.fd = conn_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
                    perror("epoll_ctl");
                    close(conn_fd);
                    continue;
                }
            } else {
                client_service(events[i].data.fd, &time_start);
                // served++;
            }
        }
    }
    close(epoll_fd);
    fclose(log_file);
    if (semctl(semaphore_id, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
