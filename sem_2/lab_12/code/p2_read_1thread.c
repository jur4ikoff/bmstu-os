#include <fcntl.h>
#include <unistd.h>

int main()
{
  char c;
  int fl1 = 1, fl2 = 1; 
  
  int fd1 = open("alphabet.txt",O_RDONLY);
  int fd2 = open("alphabet.txt",O_RDONLY);
  
  while((fl1 == 1) && (fl2 == 1))
  {
    if ((fl1 = read(fd1,&c,1)) == 1)
      write(1,&c,1);
    if (fl1 == 1 && ((fl2 = read(fd2,&c,1)) == 1))
      write(1,&c,1);
  }
  return 0;
}