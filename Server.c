#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define USERNAME_LEN 32
#define PASSWORD_LEN 32

typedef struct {
    int socket;
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    int logged_in;
} Client;

Client clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void clear_buffer(char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
}

void broadcast_message(const char *message, int exclude_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket != exclude_socket && clients[i].logged_in) {
            if (send(clients[i].socket, message, strlen(message), 0) < 0) {
                perror("Send failed");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void private_message(const char *message, const char *recipient) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (strcmp(clients[i].username, recipient) == 0 && clients[i].logged_in) {
            send(clients[i].socket, message, strlen(message), 0);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int handle_registration_login(int client_socket, char *username) {
    char buffer[BUFFER_SIZE], password[PASSWORD_LEN];
    int choice;

    sprintf(buffer, "1. Register\n2. Login\n3. Exit\n");
    send(client_socket, buffer, strlen(buffer), 0);

    clear_buffer(buffer);
    recv(client_socket, buffer, sizeof(buffer), 0);
    choice = atoi(buffer);

    if (choice == 1) {
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < num_clients; i++) {
            if (strcmp(clients[i].username, username) == 0) {
                pthread_mutex_unlock(&clients_mutex);
                send(client_socket, "Username already exists\n", 25, 0);
                return 0;
            }
        }
        send(client_socket, "Create password: ", 17, 0);
        clear_buffer(password);
        recv(client_socket, password, sizeof(password), 0);
        password[strcspn(password, "\n")] = '\0';

        strcpy(clients[num_clients].username, username);
        strcpy(clients[num_clients].password, password);
        clients[num_clients].socket = client_socket;
        clients[num_clients].logged_in = 1;
        num_clients++;
        pthread_mutex_unlock(&clients_mutex);

        send(client_socket, "Registration successful\n", 25, 0);
        return 1;
    } else if (choice == 2) {
        for (int i = 0; i < num_clients; i++) {
            if (strcmp(clients[i].username, username) == 0) {
                send(client_socket, "Enter password: ", 16, 0);
                clear_buffer(password);
                recv(client_socket, password, sizeof(password), 0);
                password[strcspn(password, "\n")] = '\0';

                if (strcmp(clients[i].password, password) == 0) {
                    clients[i].socket = client_socket;
                    clients[i].logged_in = 1;
                    send(client_socket, "Login successful\n", 18, 0);
                    return 1;
                } else {
                    send(client_socket, "Invalid password\n", 18, 0);
                    return 0;
                }
            }
        }
        send(client_socket, "Invalid username\n", 18, 0);
        return 0;
    } else {
        close(client_socket);
        pthread_exit(NULL);
    }
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    char username[USERNAME_LEN];
    int logged_in = 0;

    send(client_socket, "Enter username: ", 16, 0);
    clear_buffer(username);
    recv(client_socket, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = '\0';

    while (!logged_in) {
        logged_in = handle_registration_login(client_socket, username);
    }

    while (1) {
        clear_buffer(buffer);
        sprintf(buffer, "1. Broadcast message\n2. Private message\n3. Exit\n");
        send(client_socket, buffer, strlen(buffer), 0);

        clear_buffer(buffer);
        recv(client_socket, buffer, sizeof(buffer), 0);
        int choice = atoi(buffer);

        if (choice == 1) {
            send(client_socket, "Enter message: ", 15, 0);
            clear_buffer(buffer);
            recv(client_socket, buffer, sizeof(buffer), 0);
            buffer[strcspn(buffer, "\n")] = '\0';

            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "[%s]: %s", username, buffer);
            broadcast_message(message, client_socket);
        } else if (choice == 2) {
            send(client_socket, "Enter recipient username: ", 26, 0);
            char recipient[USERNAME_LEN];
            clear_buffer(recipient);
            recv(client_socket, recipient, sizeof(recipient), 0);
            recipient[strcspn(recipient, "\n")] = '\0';

            send(client_socket, "Enter message: ", 15, 0);
            clear_buffer(buffer);
            recv(client_socket, buffer, sizeof(buffer), 0);
            buffer[strcspn(buffer, "\n")] = '\0';

            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "[%s to %s]: %s", username, recipient, buffer);
            private_message(message, recipient);
        } else {
            break;
        }
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    printf("Server is running...\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_create(&tid, NULL, handle_client, &client_socket);
    }

    close(server_socket);
    return 0;
}
