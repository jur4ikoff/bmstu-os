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
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_t t;

  char c;
  int fl = 1;
  
  int fd1 = open("alphabet.txt",O_RDONLY);
  int fd2 = open("alphabet.txt",O_RDONLY);

  if (pthread_create(&t, &attr, thread_func, &fd1) != 0)
  {
    perror("pthread_create");
    exit(1);
  }
  
  while (fl == 1)
  {
    if ((fl = read(fd2,&c,1)) == 1)
      write(1,&c,1);
  }
  return 0;
}