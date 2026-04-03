#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define TOTAL_CONNECTIONS 450

int main() {
    struct sockaddr_in server_addr;
    srand(time(NULL));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    for (int i = 0; i < TOTAL_CONNECTIONS; i++) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            continue;
        }

        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect");
            close(sockfd);
            continue;
        }
        char type;
        if (i < 10) 
            type = 'P';
        else if (i % 4 == 0) 
            type = 'C';
        else 
            type = 'P';
        
        if (send(sockfd, &type, 1, 0) == -1) {
            perror("send");
            continue;
        }

        if (type == 'P') {
            char response[10] = {0};
            if (recv(sockfd, response, sizeof(response) - 1, 0) == -1) {
                perror("recv");
            }
            printf("[%d] Client: Sent 'P', rcv : %s\n", i, response);
        } 
        else {
            char data;
            int bytes_received = recv(sockfd, &data, 1, 0);
            if (bytes_received > 0) {
                printf("[%d] Client: Sent 'C', rcv : %c\n", i, data);
            } else {
                printf("[%d] Client: Sent 'C', but queue was empty\n", i);
            }
        }

        close(sockfd);        
        usleep(5000); 
    }

    return 0;
}