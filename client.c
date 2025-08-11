#include <stdio.h>            
#include <string.h>           
#include <stdlib.h>           // Standard library, used for memory management, conversions, and process control (e.g., exit)
#include <unistd.h>           // POSIX OS API, used here for closing sockets and managing file descriptors
#include <arpa/inet.h>        // Networking library, used for internet operations such as `socket` creation, and handling IP addresses
#include <pthread.h>          

#define BUFFER_SIZE 1024      // maximum message length

int server_socket;            // Global variable to hold the server socket descriptor, used for sending and receiving data
char buffer[BUFFER_SIZE];     // Buffer to store messages, used for both receiving & sending messages

// Thread function to receive messages from the server
void *receive_handler(void *arg) {
    while (1) {                                      // Infinite loop to keep checking for incoming messages
        memset(buffer, 0, BUFFER_SIZE);              // Clears the buffer to remove any previous data
        int receive = recv(server_socket, buffer, sizeof(buffer), 0); // Receives message from the server into buffer
        if (receive > 0) {                           // If message was received successfully
            printf("%s\n", buffer);                  // Prints the received message
        } else if (receive == 0) {                   // If receive returns 0, server closed the connection
            printf("Server disconnected.\n");        // Inform user that the server has disconnected
            exit(0);                                 // Exit the program
        }
    }
}

int main() {
    struct sockaddr_in server_addr;                  // Structure to hold server address details
    pthread_t tid;                                   // Thread identifier for the receive_handler thread

    server_socket = socket(AF_INET, SOCK_STREAM, 0); // Creates a socket (IPv4, TCP connection, default protocol)
    server_addr.sin_family = AF_INET;                // Sets address family to IPv4
    server_addr.sin_port = htons(8080);              // Sets port number to 8080, `htons` converts it to network byte order
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Sets IP address to localhost (127.0.0.1)

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) { // Connects to server
        printf("Connection failed!\n");              // Prints error message if connection fails
        return -1;                                   // Exits program with an error code
    }

    // Creates a thread to handle incoming messages from the server
    pthread_create(&tid, NULL, receive_handler, NULL);

    // Loop to continuously read user input and send to server
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);              // Clears buffer to remove any previous input
        fgets(buffer, BUFFER_SIZE, stdin);           // Reads input from user into buffer
        buffer[strcspn(buffer, "\n")] = '\0';        // Removes newline character from the input
        send(server_socket, buffer, strlen(buffer), 0); // Sends the input data to the server
    }

    close(server_socket);                            // Closes the server socket after exiting the loop
    return 0;                                        // Returns 0 to indicate successful execution
}
