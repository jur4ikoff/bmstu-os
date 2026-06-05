//#include <sys/types.h>
//#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>

#include <sys/time.h>

#define PORT 9876
#define ITER 1000

int fl = 1;
void sig_handler(int sig_num)
{
    fl = 0;
}

int main()
{
    int sock_fd;
    struct sockaddr_in srvr_addr;
    char clnt_type = 'p', diff = 19, buf[3];
    ssize_t read_len = 0;
    
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("signal");
        printf("errno: %d\n", errno);
        exit(1);
    }

    memset(&srvr_addr, 0, sizeof(srvr_addr));
    srvr_addr.sin_family = AF_INET;
    srvr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvr_addr.sin_port = htons(PORT);

    for (int i = 0; (i < ITER) && fl; i++)
    {
        if ((i % 5) == 0)
            clnt_type = clnt_type ^ diff;
        
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1)
        {
            perror("socket");
            printf("errno: %d\n", errno);
            close(sock_fd);
            exit(1);
        }
        if (connect(sock_fd, (struct sockaddr *)&srvr_addr, sizeof(srvr_addr)) == -1)
        {
            perror("connect");
            printf("%d) errno: %d\n", i, errno);
            close(sock_fd);
            exit(1);
        }

        if (write(sock_fd, &clnt_type, sizeof(clnt_type)) == -1)
        {
            perror("write");
            printf("errno: %d\n", errno);
            close(sock_fd);
            exit(1);
        }
        
        read_len = read(sock_fd, buf, sizeof(buf));
        if (read_len == -1)
        {
            perror("read");
            printf("errno: %d\n", errno);
            close(sock_fd);
            exit(1);
        }
        else if (read_len == 0)
        {
            printf("connection terminated\n");
            close(sock_fd);\
            exit(1);
        }
        else
        {
            buf[read_len - 1] = '\0';

            printf("Type: %c. Result: %s\n", clnt_type, buf);
        }

        close(sock_fd);
    }

    return 0;
}
