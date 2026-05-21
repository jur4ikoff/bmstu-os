#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void *thread_func(void *arg)
{
  char c;
  int fl = 1;
  int fd = *((int *)arg);

  while (fl == 1)
  {
    if ((fl = read(fd, &c, 1)))
      write(1, &c, 1);
  }
  return NULL;
}

int main()
{
  pthread_t t1, t2;
  
  int fd1 = open("alphabet.txt",O_RDONLY);
  int fd2 = open("alphabet.txt",O_RDONLY);

  if (pthread_create(&t1, NULL, thread_func, &fd1) != 0)
  {
    perror("pthread_create");
    exit(1);
  }
  if (pthread_create(&t2, NULL, thread_func, &fd2) != 0)
  {
    perror("pthread_create");
    exit(1);
  }

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  
  return 0;
}