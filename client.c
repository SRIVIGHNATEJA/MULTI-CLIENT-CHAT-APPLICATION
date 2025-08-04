#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Connection failed!\n");
        return -1;
    }

    printf("Connected to server!\n");
    memset(buffer, 0, BUFFER_SIZE);
    recv(server_socket, buffer, sizeof(buffer), 0);
    printf("%s\n", buffer);

    close(server_socket);
    return 0;
}
