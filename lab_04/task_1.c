// Производитель, потребитель
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

#define BIN_SEM 0
#define BUFFER_EMPTY 1
#define BUFFER_FULL 2

struct sembuf start_consume[] = {{BUFFER_FULL, -1, 0}, {BIN_SEM, -1, 0}};
struct sembuf stop_consume[] = {{BIN_SEM, 1, 0}, {BUFFER_EMPTY, 1, 0}};
struct sembuf start_produce[] = {{BUFFER_EMPTY, -1, 0}, {BIN_SEM, -1, 0}};
struct sembuf stop_produce[] = {{BIN_SEM, 1, 0}, {BUFFER_FULL, 1, 0}};

int flag = 1;
int alphabet_cycles = 0;

void sig_handler(int sig_num)
{
    printf("pid: %d; signal: %d\n", getpid(), sig_num);
    flag = 0;
    exit(0);
}

void producer(char **pr_ptr, char *letter, int sem_id)
{
    while (flag)
    {
        if (semop(sem_id, start_produce, 2) == -1)
        {
            perror("semop");
            exit(1);
        }

        **pr_ptr = *letter;
        (*pr_ptr)++;

        printf("Producer %d: %c\n", getpid(), *letter);
        (*letter)++;

        if (*letter > 'z')
        {
            (*letter) = 'a';
        }

        if (semop(sem_id, stop_produce, 2) == -1)
        {
            perror("semop");
            exit(1);
        }
    }
}

void consumer(char **cn_ptr, int sem_id)
{
    while (flag)
    {
        if (semop(sem_id, start_consume, 2) == -1)
        {
            perror("semop");
            exit(1);
        }

        printf("Consumer %d: %c\n", getpid(), **cn_ptr);

        if (**cn_ptr == 'z')
        {
            ++alphabet_cycles;
            if (alphabet_cycles == 2)
            {
                kill(0, SIGALRM);
            }
        }
        (*cn_ptr)++;

        if (semop(sem_id, stop_consume, 2) == -1)
        {
            perror("semop");
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    pid_t array_pids[12];
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int shm_id, sem_id, w;
    char *addr, *data_ptr;
    key_t sem_key, shm_key;

    if (signal(SIGALRM, sig_handler) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }

    sem_key = ftok("./key.txt", 1);
    if (sem_key == -1)
    {
        perror("ftok (sem)");
        exit(1);
    }
    if ((sem_id = semget(sem_key, 3, IPC_CREAT | perms)) == -1)
    {
        perror("semget");
        exit(1);
    }
    if (semctl(sem_id, BIN_SEM, SETVAL, 1) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (semctl(sem_id, BUFFER_EMPTY, SETVAL, 26) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (semctl(sem_id, BUFFER_FULL, SETVAL, 0) == -1)
    {
        perror("semctl");
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
    if ((addr = shmat(shm_id, NULL, 0)) == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    char **pr_ptr = (char **)addr;
    char **cn_ptr = (char **)(addr + sizeof(char *));
    char *letter = (char *)(addr + 2 * sizeof(char *));

    *letter = 'a';
    char *buffer_start = (char *)(letter + 1);
    *pr_ptr = buffer_start;
    *cn_ptr = buffer_start;

    for (size_t i = 0; i < 12; i++)
    {
        if ((array_pids[i] = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }
        if (array_pids[i] == 0)
        {
            if (i < 6)
            {
                producer(pr_ptr, letter, sem_id);
            }
            else
            {
                consumer(cn_ptr, sem_id);
            }
        }
    }

    for (size_t i = 0; i < 12; i++)
    {
        int wstatus;
        pid_t w = waitpid(array_pids[i], &wstatus, 0);
        if (w == -1)
        {
            perror("wait");
            exit(1);
        }
        if (WIFEXITED(wstatus))
        {
            printf("Child %d exited. Code: %d\n", w, WEXITSTATUS(wstatus));
        }
        if (WIFSIGNALED(wstatus))
        {
            printf("Child %d terminated. Signal #: %d\n", w, WTERMSIG(wstatus));
        }
        if (WIFSTOPPED(wstatus))
        {
            printf("Child %d stopped. Signal #: %d\n", w, WSTOPSIG(wstatus));
        }
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
