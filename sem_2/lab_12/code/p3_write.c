#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main() 
{
  struct stat statbuf;
  int fd1 = open("q.txt",O_RDWR);
  int fd2 = open("q.txt",O_RDWR);
  stat("q.txt", &statbuf);
  printf("inode: %lu;size: %ld; blksize: %ld\n", statbuf.st_ino, statbuf.st_size, statbuf.st_blksize);
  
  for(char c = 'a'; c <= 'z'; c++)
  {
  	if (c%2)
		  write(fd1, &c, 1);
  	else
		  write(fd2, &c, 1);
    stat("q.txt", &statbuf);
    printf("inode: %lu;size: %ld; blksize: %ld\n", statbuf.st_ino, statbuf.st_size, statbuf.st_blksize);
  }
  close(fd1);
  close(fd2);
  return 0;
}