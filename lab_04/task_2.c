// Читатель писатель

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <signal.h>

#define READERS_CNT 2
#define WRITERS_CNT 3

#define ACTIVE_READER 0
#define ACTIVE_WRITER 1
#define WRITE_COUNT   2
#define READ_COUNT    3

struct sembuf start_read[5] = {
    {READ_COUNT, 1, 0},
    {ACTIVE_WRITER, 0, 0},
    {WRITE_COUNT, 0, 0},
    {ACTIVE_READER, 1, 0},
    {READ_COUNT, -1, SEM_UNDO | IPC_NOWAIT}
};
struct sembuf stop_read[1] = {
    {ACTIVE_READER, -1, SEM_UNDO | IPC_NOWAIT}
};
struct sembuf start_write[5] = {
    {WRITE_COUNT, 1, 0},
    {ACTIVE_READER, 0, 0},
    {ACTIVE_WRITER, 0, 0},
    {ACTIVE_WRITER, 1, 0},
    {WRITE_COUNT, -1, SEM_UNDO | IPC_NOWAIT}
};
struct sembuf stop_write[1] = {
    {ACTIVE_WRITER, -1, SEM_UNDO | IPC_NOWAIT}
};
int flag = 1;
int cnt = 0;
void sig_handler(int sig_num) 
{
    if (getpgrp() == getpid())
        return;
    flag = 0;
    printf("pid: %d, signal: %d\n", getpid(), sig_num);
    exit(0);
}
void reader(char *const addr, const int semid, const int rdid) {
    while (flag) 
    {
        if (semop(semid, start_read, 5) == -1) 
        {
            perror("semop");
            printf("errno reader start: %d\n", errno);
            exit(1);
        }
        printf("Reader %d: - %c\n", getpid(), *addr);
        if (semop(semid, stop_read, 1) == -1) 
        {
            perror("semop");
            printf("errno reader stop: %d\n", errno);
            exit(1);
        }
    }
    exit(0);
}
void writer(char *const addr, const int semid, const int wrid) {
    while (flag) 
    {
        if (semop(semid, start_write, 5) == -1) 
        {
            perror("semop");
            printf("errno writer start: %d\n", errno);
            exit(1);
        }
        if (*addr == 'z')
        {
            if (cnt == 1)
                kill(0, SIGALRM);
            *addr = 'a';
            cnt++;
        }
        else
        {
            ++(*addr);
        }
        printf("Writer %d: - %c\n", getpid(), *addr);
        if (semop(semid, stop_write, 1) == -1) 
        {
            perror("semop");
            printf("errno writer stop: %d\n", errno);
            exit(1);
        }
    }
    exit(0);
}
int main(void) {
    char *addr;
    key_t sem_key, shm_key;
    int shm_id, sem_id, cpid;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int child_pids[READERS_CNT + WRITERS_CNT];

    if (signal(SIGALRM, sig_handler) == SIG_ERR) 
    {
        perror("signal");
        exit(1);
    }
    sem_key = ftok("./key.txt", 1);
    if (sem_key == -1)
    {
        perror("ftok");
        exit(1);
    }
    if ((sem_id = semget(sem_key, 4,  IPC_CREAT | perms)) == -1)
    {
        perror("ftok (sem)");
        exit(1);
    }
    if (semctl(sem_id, ACTIVE_READER, SETVAL, 0) == -1)
    {
        perror("semctl1");
        exit(1);
    }
    if (semctl(sem_id, ACTIVE_WRITER, SETVAL, 0) == -1)
    {
        perror("semctl2");
        exit(1);
    }
    if (semctl(sem_id, WRITE_COUNT, SETVAL, 0) == -1)
    {
        perror("semctl3");
        exit(1);
    }
    if (semctl(sem_id, READ_COUNT, SETVAL, 0) == -1)
    {
        perror("semctl4");
        exit(1);
    }
    shm_key = ftok("./key.txt", 2);
    if (shm_key == -1)
    {
        perror("ftok (shm)");
        exit(1);
    }
    if ((shm_id = shmget(shm_key, 512, IPC_CREAT | perms)) == -1)
    {
        perror("shmget");
        exit(1);
    }
    if ((addr = shmat(shm_id, NULL, 0)) == (void*)-1)
    {
        perror("shmat");
        exit(1);
    }
    *addr = 'a';
    printf("Writer %d: - %c\n", getpid(), *addr);
    for (size_t i = 0; i < READERS_CNT + WRITERS_CNT; i++) 
    {
        if ((cpid = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }
        child_pids[i] = cpid;
        if (cpid == 0)
        {
            if (i % 3 == 0)
                reader(addr, sem_id, i);
            else
                writer(addr, sem_id, i);
        }
    }
    for (size_t i = 0; i < READERS_CNT + WRITERS_CNT; i++)
    {
        int wstatus;
        pid_t w = waitpid(child_pids[i], &wstatus, 0);
        if (w == -1)
        {
            perror("wait");
            exit(1);
        }
        printf("Child #%zu (PID: %d) exited.\n", i, w);
        if (WIFEXITED(wstatus))
            printf("Child #%zu exited. Code: %d\n", i, WEXITSTATUS(wstatus));
        if (WIFSIGNALED(wstatus))
            printf("Child #%zu terminated. Signal #: %d\n", i, WTERMSIG(wstatus));
        if (WIFSTOPPED(wstatus))
            printf("Child #%zu stopped. Signal #: %d\n", i, WSTOPSIG(wstatus));
        puts("");
    }
    if (shmdt(addr) == -1)
    {
        perror("shmdt");
        exit(1);
    }
    if (semctl(sem_id, 0, IPC_RMID) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (shmctl(shm_id, IPC_RMID, 0) == -1)
    {
        perror("shmctl");
        exit(1);
    }
    return 0;
}
