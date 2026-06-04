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
    //char file_name[32];
    //FILE *fptr;
    //struct timespec p_starttime, p_endtime;
    //long seconds, nanoseconds;
    //double msec, msec_sum = 0.0;
    //int i = 0;

    //sprintf(file_name, "%d.log", getpid());
    //fptr = fopen(file_name, "w");
    // if (fptr == NULL)
    // {
    //     perror("fopen");
    //     exit(1);
    // }
    
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("signal");
        printf("errno: %d\n", errno);
        //fclose(fptr);
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
            //fclose(fptr);
            exit(1);
        }
        if (connect(sock_fd, (struct sockaddr *)&srvr_addr, sizeof(srvr_addr)) == -1)
        {
            perror("connect");
            printf("%d) errno: %d\n", i, errno);
            close(sock_fd);
            //fclose(fptr);
            exit(1);
        }

        //clock_gettime(CLOCK_REALTIME, &p_starttime);
        if (write(sock_fd, &clnt_type, sizeof(clnt_type)) == -1)
        {
            perror("write");
            printf("errno: %d\n", errno);
            close(sock_fd);
            //fclose(fptr);
            exit(1);
        }
        
        read_len = read(sock_fd, buf, sizeof(buf));
        //clock_gettime(CLOCK_REALTIME, &p_endtime);
        if (read_len == -1)
        {
            perror("read");
            printf("errno: %d\n", errno);
            close(sock_fd);
            //fclose(fptr);
            exit(1);
        }
        else if (read_len == 0)
        {
            printf("connection terminated\n");
            close(sock_fd);\
            //fclose(fptr);
            exit(1);
        }
        else
        {
            buf[read_len - 1] = '\0';

            printf("Type: %c. Result: %s\n", clnt_type, buf);
        }

        // msec = (p_endtime.tv_sec - p_starttime.tv_sec) * 1.0e+3 + (p_endtime.tv_nsec - p_starttime.tv_nsec) / 1.0e+6;
        // msec_sum += msec;
        // fprintf(fptr, "TYPE: %c. RESULT: %s RQ_TIME:%lf \n", clnt_type, buf, msec);

        close(sock_fd);
    }

    // printf("AVG_TIME: %lf\n", (i > 0) ? (msec_sum / i) : 0.0);

    //fclose(fptr);
    return 0;
}
