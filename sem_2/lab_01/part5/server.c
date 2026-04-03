//#include <sys/types.h>
//#include <sys/socket.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ipc.h>


#include <sys/epoll.h>

#define SEM_EMPTY 0
#define SEM_FULL 1

struct sembuf produce_begin[1] = {{SEM_EMPTY, -1, 0}};
struct sembuf produce_end[1] = {{SEM_FULL, 1, 0}};
struct sembuf consume_begin[1] = {{SEM_FULL, -1, 0}};
struct sembuf consume_end[1] = {{SEM_EMPTY, 1, 0}};

#define PORT 1337
#define RESPOND_BUF_SIZE 3

#define LISTEN_NUM 5
#define MAX_EVENTS 10

#define N 26

struct timespec timeout = {.tv_sec = 0, .tv_nsec = 500000};

typedef struct
{
    int sem_fd;
    int conn_fd;
    char *buf;
    char *letter_ptr;
    int *producer_index_ptr;
    int *consumer_index_ptr;
    FILE *fptr;
    struct timespec starttime;
} data_t;


int fl = 1;
void sig_handler(int sig_num)
{
    fl = 0;
}

void producer(int sem_fd, int conn_fd, char *buf, int *producer_index_ptr, char *letter_ptr)
{
    char msg_buf[RESPOND_BUF_SIZE] = "ER";
    if (semtimedop(sem_fd, produce_begin, 1, &timeout) == -1)
    {
        if (errno != EAGAIN)
        {
            perror("semtimedop");
            printf("errno: %d\n", errno);
            exit(1);
        }
    }
    else
    {
        buf[*producer_index_ptr] = *letter_ptr;
        if ((*letter_ptr) >= 'z')
            *letter_ptr = 'a';
        else
            (*letter_ptr)++;
        (*producer_index_ptr)++;
        if ((*producer_index_ptr) >= N)
            *producer_index_ptr = 0;
        if (semtimedop(sem_fd, produce_end, 1, &timeout) == -1)
        {
            if (errno != EAGAIN)
            {
                perror("semtimedop");
                printf("errno: %d\n", errno);
                exit(1);
            }
        }
        else
        {
            strcpy(msg_buf, "OK");
        }
    }
    if (write(conn_fd, &msg_buf, sizeof(msg_buf)) == -1)
    {
        perror("write");
        printf("errno: %d\n", errno);
        exit(1);
    }
}

void consumer(int sem_fd, int conn_fd, char *buf, int *consumer_index_ptr)
{
    char msg_buf[RESPOND_BUF_SIZE] = "";
    if (semtimedop(sem_fd, consume_begin, 1, &timeout) == -1)
    {
        if (errno == EAGAIN)
        {
            strcpy(msg_buf, "ER");
        }
        else
        {
            perror("semtimedop");
            printf("errno: %d\n", errno);
            exit(1);
        }
    }
    else
    {
        msg_buf[0] = buf[*consumer_index_ptr];
        (*consumer_index_ptr)++;
        if ((*consumer_index_ptr) >= N)
            (*consumer_index_ptr) = 0;
        if (semtimedop(sem_fd, consume_end, 1, &timeout) == -1)
        {
            if (errno == EAGAIN)
            {
                strcpy(msg_buf, "ER");
            }
            else
            {
                perror("semtimedop");
                printf("errno: %d\n", errno);
                exit(1);
            }
        }
    }
    if (write(conn_fd, &msg_buf, sizeof(msg_buf)) == -1)
    {
        perror("write");
        printf("errno: %d\n", errno);
        exit(1);
    }
}

void worker(data_t args)
{
    char type;
    ssize_t read_len = 0;
    struct timespec endtime;
    double usec;

    if ((read_len = read(args.conn_fd, &type, sizeof(type))) > 0)
    {
        switch(type)
        {
            case 'p':
                producer(args.sem_fd, args.conn_fd, args.buf, args.producer_index_ptr, args.letter_ptr);
                break;
            case 'c':
                consumer(args.sem_fd, args.conn_fd, args.buf, args.consumer_index_ptr);
                break;
            default:
                break;
        }
    }

    close(args.conn_fd);
    if (read_len == -1)
    {
        perror("read_len");
        printf("errno: %d\n", errno);
        exit(1);
    }

    clock_gettime(CLOCK_REALTIME, &endtime);
    usec = (endtime.tv_sec - args.starttime.tv_sec) * 1.0e+6 + (endtime.tv_nsec - args.starttime.tv_nsec) / 1.0e+3;
    fprintf(args.fptr, "%lf \n", usec);
}

int main(void)
{
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    key_t semkey;
    int sem_fd;
    char letter = 'a';
    int producer_index = 0, consumer_index = 0;
    char buf[N];
    FILE *fptr;
    struct epoll_event ev, events[MAX_EVENTS];
    int sock_fd, conn_fd, nfds, epollfd;
    struct sockaddr_in srvr_addr, clnt_addr;
    socklen_t clnt_len = sizeof(clnt_addr);
    data_t worker_args;

    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("signal");
        printf("errno: %d\n", errno);
        exit(1);
    }
    semkey = ftok("ftok_key", 1);
    if (semkey == -1)
    {
        perror("semkey");
        printf("errno: %d\n", errno);
        exit(1);
    }
    sem_fd = semget(semkey, 2, IPC_CREAT | perms);
    if (sem_fd == -1)
    {
        perror("semget");
        printf("errno: %d\n", errno);
        exit(1);
    }
    if (semctl(sem_fd, SEM_FULL, SETVAL, 0) == -1)
    {
        perror("semctl");
        printf("errno: %d\n", errno);
        exit(1);
    }
    if (semctl(sem_fd, SEM_EMPTY, SETVAL, N) == -1)
    {
        perror("semctl");
        printf("errno: %d\n", errno);
        exit(1);
    }

    fptr = fopen("epoll_server.log", "w");
    if (fptr == NULL)
    {
        perror("fopen");
        printf("errno: %d\n", errno);
        exit(1);
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        fclose(fptr);
        perror("socket");
        printf("errno: %d\n", errno);
        exit(1);
    }
    memset(&srvr_addr, 0, sizeof(srvr_addr));
    srvr_addr.sin_family = AF_INET;
    srvr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvr_addr.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(sock_fd, (struct sockaddr *)&srvr_addr, sizeof(srvr_addr)) < 0)
    {
        fclose(fptr);
        close(sock_fd);
        perror("bind");
        printf("errno: %d\n", errno);
        exit(1);
    }
    if (listen(sock_fd, LISTEN_NUM) == -1)
    {
        fclose(fptr);
        close(sock_fd);
        perror("listen");
        printf("errno: %d\n", errno);
        exit(1);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        fclose(fptr);
        close(sock_fd);
        perror("epoll_create1");
        printf("errno: %d\n", errno);
        exit(1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = sock_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock_fd, &ev) == -1)
    {
        fclose(fptr);
        close(sock_fd);
        perror("epoll_ctl");
        printf("errno: %d\n", errno);
        exit(1);
    }

    worker_args.sem_fd = sem_fd;
    worker_args.consumer_index_ptr = &consumer_index;
    worker_args.producer_index_ptr = &producer_index;
    worker_args.buf = buf;
    worker_args.letter_ptr = &letter;
    worker_args.fptr = fptr;

    while (fl)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            fclose(fptr);
            close(sock_fd);
            perror("epoll_wait");
            printf("errno: %d\n", errno);
            exit(1);
        }

        clock_gettime(CLOCK_REALTIME, &(worker_args.starttime));
        for (int n = 0; n < nfds; n++)
        {
            if (events[n].data.fd == sock_fd)
            {
                conn_fd = accept(sock_fd, (struct sockaddr *)&clnt_addr, &clnt_len);
                if (conn_fd == -1)
                {
                    fclose(fptr);
                    close(sock_fd);
                    perror("accept");
                    printf("errno: %d\n", errno);
                    exit(1);
                }

                ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                ev.data.fd = conn_fd;
                setnonblocking(conn_fd);
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_fd, &ev) == -1)
                {
                    fclose(fptr);
                    close(sock_fd);
                    perror("epoll_ctl");
                    printf("errno: %d\n", errno);
                    exit(1);
                }
            }
            else
            {
                worker_args.conn_fd = events[n].data.fd;
                worker(worker_args);
            }
        }
    }
   

    fclose(fptr);
    close(sock_fd);
    if (semctl(sem_fd, IPC_RMID, 0) == -1)
    {
        perror("shmctl");
        printf("errno: %d\n", errno);
        exit(1);
    }
    return 0;
}