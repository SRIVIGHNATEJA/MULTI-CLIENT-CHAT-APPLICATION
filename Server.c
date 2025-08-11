#include <stdio.h>                // Standard I/O functions
#include <string.h>               // String handling functions
#include <stdlib.h>               // Standard library functions (e.g., malloc, free)
#include <unistd.h>               // POSIX operating system API (e.g., close)
#include <pthread.h>              // POSIX threads for multithreading
#include <arpa/inet.h>            // Networking functions (e.g., socket, bind)

#define MAX_CLIENTS 10            
#define BUFFER_SIZE 1024          // Buffer size for messages
#define USERNAME_LEN 32           // Max username length
#define PASSWORD_LEN 32           // Max password length

// Client struct to hold client-specific data
typedef struct {
    int socket;                   // Socket file descriptor for the client
    char username[USERNAME_LEN];  // Username
    char password[PASSWORD_LEN];  // Password
    int logged_in;                // Login status (1 = logged in, 0 = not)
} Client;

Client clients[MAX_CLIENTS];      // Array to store client data
int num_clients = 0;              // Current number of clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to synchronize client array

// Clear buffer by setting its contents to zero
void clear_buffer(char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
}

// Send a message to all clients except the sender
void broadcast_message(const char *message, int exclude_socket) {
    pthread_mutex_lock(&clients_mutex); // Lock for thread safety
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket != exclude_socket && clients[i].logged_in) { // Send if logged in and not sender
            if (send(clients[i].socket, message, strlen(message), 0) < 0) { // Send message
                perror("Send failed");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex); // Unlock after sending
}

// Send a private message to a specific client
void private_message(const char *message, const char *recipient) {
    pthread_mutex_lock(&clients_mutex); // Lock for thread safety
    for (int i = 0; i < num_clients; i++) {
        if (strcmp(clients[i].username, recipient) == 0 && clients[i].logged_in) { // Find recipient
            send(clients[i].socket, message, strlen(message), 0); // Send message
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex); // Unlock after sending
}

// Handle client registration and login
int handle_registration_login(int client_socket, char *username) {
    char buffer[BUFFER_SIZE], password[PASSWORD_LEN];
    int choice;
    int authenticated = 0;

    sprintf(buffer, "1. Register\n2. Login\n3. Exit\n"); // Send options to client
    send(client_socket, buffer, strlen(buffer), 0);
    
    clear_buffer(buffer); // Clear buffer for receiving
    recv(client_socket, buffer, sizeof(buffer), 0);
    choice = atoi(buffer); // Convert choice to integer

    if (choice == 1) { // Registration
        pthread_mutex_lock(&clients_mutex); // Lock for thread safety
        for (int i = 0; i < num_clients; i++) { // Check if username exists
            if (strcmp(clients[i].username, username) == 0) {
                pthread_mutex_unlock(&clients_mutex); // Unlock if exists
                send(client_socket, "Username already exists\n", 25, 0);
                return 0;
            }
        }
        send(client_socket, "Create password: ", 17, 0); // Prompt for password
        clear_buffer(password);
        recv(client_socket, password, sizeof(password), 0);
        password[strcspn(password, "\n")] = '\0'; // Remove newline

        // Store client data
        strcpy(clients[num_clients].username, username);
        strcpy(clients[num_clients].password, password);
        clients[num_clients].socket = client_socket;
        clients[num_clients].logged_in = 1;
        num_clients++;

        pthread_mutex_unlock(&clients_mutex); // Unlock after registration
        send(client_socket, "Registration successful\n", 25, 0);

        sprintf(buffer, "Do you want to login? (1 for Yes, 2 for No)\n"); // Prompt to login
        send(client_socket, buffer, strlen(buffer), 0);
        clear_buffer(buffer);
        recv(client_socket, buffer, sizeof(buffer), 0);

        int login_choice = atoi(buffer);
        if (login_choice == 1) {
            return handle_registration_login(client_socket, username); // Redirect to login
        } else if (login_choice == 2) {
            send(client_socket, "Exiting... Goodbye!\n", 20, 0);
            close(client_socket);
            pthread_exit(NULL); // Clean exit
        } else {
            send(client_socket, "Invalid input. Returning to menu...\n", 36, 0);
            return 0;
        }

    } else if (choice == 2) { // Login
        for (int i = 0; i < num_clients; i++) { // Check username
            if (strcmp(clients[i].username, username) == 0) {
                send(client_socket, "Enter password: ", 16, 0); // Prompt for password
                clear_buffer(password);
                recv(client_socket, password, sizeof(password), 0);
                password[strcspn(password, "\n")] = '\0';

                if (strcmp(clients[i].password, password) == 0) { // Verify password
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

    } else { // Exit
        close(client_socket);
        pthread_exit(NULL);
    }
}

// Thread function for handling individual client
void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    char username[USERNAME_LEN];
    int logged_in = 0;

    send(client_socket, "Enter username: ", 16, 0); // Ask for username
    clear_buffer(username);
    recv(client_socket, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = '\0';

    // Handle login or registration
    while (!logged_in) {
        logged_in = handle_registration_login(client_socket, username);
    }

    // Main menu loop
    while (1) {
        clear_buffer(buffer);
        sprintf(buffer, "1. Broadcast message\n2. Private message\n3. Logout\n"); // Send menu
        send(client_socket, buffer, strlen(buffer), 0);

        clear_buffer(buffer);
        recv(client_socket, buffer, sizeof(buffer), 0);
        int choice = atoi(buffer);

        if (choice == 1) { // Broadcast message
            send(client_socket, "Enter message: ", 15, 0);
            clear_buffer(buffer);
            recv(client_socket, buffer, sizeof(buffer), 0);
            buffer[strcspn(buffer, "\n")] = '\0';

            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "[%s]: %s", username, buffer); // Format message
            broadcast_message(message, client_socket); // Broadcast message

        } else if (choice == 2) { // Private message
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
            snprintf(message, sizeof(message), "[%s to %s]: %s", username, recipient, buffer); // Format message
            private_message(message, recipient); // Send private message

        } else if (choice == 3) { // Logout
            send(client_socket, "Logged out\n", 11, 0);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < num_clients; i++) {
                if (clients[i].socket == client_socket) {
                    clients[i].logged_in = 0; // Mark as logged out
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            close(client_socket);
            pthread_exit(NULL);
        }
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    server_socket = socket(AF_INET, SOCK_STREAM, 0); // Create socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)); // Bind socket
    listen(server_socket, 5); // Listen for connections

    printf("Server is running...\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len); // Accept connection
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_create(&tid, NULL, handle_client, &client_socket); // Create thread for client
    }

    close(server_socket); // Close server socket
    return 0;
}
