#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int server_socket;
char buffer[BUFFER_SIZE];

void *receive_handler(void *arg) {
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int receive = recv(server_socket, buffer, sizeof(buffer), 0);
        if (receive > 0) {
            printf("%s\n", buffer);
        } else if (receive == 0) {
            printf("Server disconnected.\n");
            exit(0);
        }
    }
}

int main() {
    struct sockaddr_in server_addr;
    pthread_t tid;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Connection failed!\n");
        return -1;
    }

    pthread_create(&tid, NULL, receive_handler, NULL);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        send(server_socket, buffer, strlen(buffer), 0);
    }

    close(server_socket);
    return 0;
}
