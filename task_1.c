#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define BIN_SEM 0
#define BUFFER_EMPTY 1
#define BUFFER_FULL 2
#define PRODUCERS_CNT 3
#define CONSUMERS_CNT 3
#define N (26 * 3)

struct sembuf p_consumer[2] = {{BUFFER_FULL, -1, 0}, {BIN_SEM, -1, 0}};
struct sembuf v_consumer[2] = {{BIN_SEM, 1, 0}, {BUFFER_EMPTY, 1, 0}};
struct sembuf p_producer[2] = {{BUFFER_EMPTY, -1, 0}, {BIN_SEM, -1, 0}};
struct sembuf v_producer[2] = {{BIN_SEM, 1, 0}, {BUFFER_FULL, 1, 0}};

void consume(int num, int sem_id, char *buf, int *cons_ptr)
{
 while (1)
 {
  semop(sem_id, p_consumer, 2);
  // printf("Consumer #%d read  buf[%d] = %c\n", num, *cons_ptr, buf[*cons_ptr]);
  printf("Consumer (id=%d) read buf[%d] = %c\n", getpid(), *cons_ptr, buf[*cons_ptr]);
  *cons_ptr = *cons_ptr == N - 1 ? 0 : *cons_ptr + 1;
  semop(sem_id, v_consumer, 2);
  sleep(rand() % 3);
 }
}

void produce(int num, int sem_id, char *buf, int *pr_ptr, char *letter)
{
 while (1)
 {
  semop(sem_id, p_producer, 2);
  buf[*pr_ptr] = *letter;
  // printf("Producer (id=%d) write to buf[%d] = %c\n", num, *pr_ptr, buf[*pr_ptr]);
  printf("Producer (id=%d) write to buf[%d] = %c\n", getpid(), *pr_ptr, buf[*pr_ptr]);
  *pr_ptr = *pr_ptr == N - 1 ? 0 : *pr_ptr + 1;
  *letter = *letter > 'z' ? 'a' : *letter + 1;
  semop(sem_id, v_producer, 2);
  sleep(rand() % 3);
 }
}

size_t create_proc(int sem_id, char *mem_ptr, char *data_ptr)
{
 int processes_cnt = 0;
 pid_t pid;
 char *letter = mem_ptr + 2 * sizeof(int);
 int *pr_ptr = (int *)(mem_ptr);
 int *cons_ptr = (int *)(mem_ptr + sizeof(int));
 *pr_ptr = *cons_ptr = 0;
 *letter = 'a';
 for (int i = 0; i < PRODUCERS_CNT; i++)
 {
  if ((pid = fork()) == -1)
  {
   perror("fork");
   exit(1);
  }
  if (pid == 0)
   produce(i + 1, sem_id, data_ptr, pr_ptr, letter);
  else
   processes_cnt++;
 }
 for (int i = 0; i < CONSUMERS_CNT; i++)
 {
  if ((pid = fork()) == -1)
  {
   perror("fork");
   exit(1);
  }
  if (pid == 0)
   consume(i + 1, sem_id, data_ptr, cons_ptr);
  else
   processes_cnt++;
 }
 return processes_cnt;
}

int main(int argc, char **argv)
{
 int flags = S_IRWXU | S_IRWXG | S_IRWXO;
 int shm_id, sem_id;
 size_t processes_cnt;
 char *addr = (char *)(-1), *data_ptr;
 srand(time(NULL));
 key_t sem_key = ftok(argv[0], 1);
 if (sem_key == -1)
 {
  perror("ftok (sem)");
  exit(1);
 }
 if ((sem_id = semget(sem_key, 3, IPC_CREAT | flags)) == -1)
 {
  perror("semget");
  exit(1);
 }
 key_t shm_key = ftok(argv[0], 2);
 if (shm_key == -1)
 {
  perror("ftokshm");
  exit(1);
 }
 if ((shm_id = shmget(shm_key, 2 * sizeof(int) + sizeof(char) + N * sizeof(char), IPC_CREAT | flags)) == -1)
 {
  perror("shmget");
  exit(1);
 }
 if ((addr = shmat(shm_id, NULL, 0)) == (void *)-1)
 {
  perror("shmat");
  exit(1);
 }
 data_ptr = addr + sizeof(int) * 2 + sizeof(char);
 if (semctl(sem_id, 0, SETVAL, 1) == -1)
 {
  perror("semctl");
  exit(1);
 }
 if (semctl(sem_id, 1, SETVAL, N) == -1)
 {
  perror("semctl");
  exit(1);
 }
 if (semctl(sem_id, 2, SETVAL, 0) == -1)
 {
  perror("semctl");
  exit(1);
 }
 processes_cnt = create_proc(sem_id, addr, data_ptr);
 for (size_t i = 0; i < processes_cnt; i++)
 {
  int status;
  pid_t childpid = wait(&status);
  if (childpid == -1)
  {
   perror("wait");
   exit(1);
  }
  printf("Child #%zu (PID: %d) exited.\n", i, childpid);
  if (WIFEXITED(status))
   printf("Child #%zu exited. Code: %d\n", i, WEXITSTATUS(status));
  if (WIFSIGNALED(status))
   printf("Child #%zu terminated. Signal #: %d\n", i, WTERMSIG(status));
  if (WIFSTOPPED(status))
   printf("Child #%zu stopped. Signal #: %d\n", i, WSTOPSIG(status));
  puts("");
 }
 if (shmdt(addr) == -1)
 {
  perror("semdt");
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
