#include "common.h"

typedef struct {
    int socket;
    char username[USERNAME_SIZE];
} Client;

// Global Variables
Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function Declarations
void *handle_client(void *arg);
void broadcast_message(const char *message, int sender_socket);
void remove_client(int socket);
int is_username_taken(const char *username);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Initialize clients array
    memset(clients, 0, sizeof(clients));

    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR option
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void *)&client_socket) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            continue;
        }
        pthread_detach(thread);
    }

    close(server_socket);
    return 0;
}

// Handle client connection
void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char username[BUFFER_SIZE];

    // get username
    int bytes_read = recv(client_socket, username, BUFFER_SIZE, 0);
    username[bytes_read ] = '\0'; // Remove newline character

    pthread_mutex_lock(&clients_mutex);
    if (is_username_taken(username)) {
        send(client_socket, "Username already taken. Disconnecting.\n", 40, 0);
        close(client_socket);
        pthread_mutex_unlock(&clients_mutex);
        return NULL;
    }

    // Add client to the list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = client_socket;
            strcpy(clients[i].username, username);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    printf("[%s] [SERVER]: %s has joined the chat.\n", time_str, username);


    // Announce new user
    snprintf(buffer, sizeof(buffer), "%.100s has joined the chat.\n", username);
    broadcast_message(buffer, client_socket);

    // Handle messages
    while (1) {
        bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';

        if (strcmp(buffer, "/quit\n") == 0) {
            break;
        }

        char message[MSG_SIZE];
        snprintf(message, sizeof(message), "[%.100s]: %.1024s", username, buffer);
        broadcast_message(message, client_socket);
    }

    snprintf(buffer, sizeof(buffer), "%.100s has disconnected.\n", username);
    broadcast_message(buffer, client_socket);
    remove_client(client_socket);

    // Server-side log for client leaving
    time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    printf("[%s] [SERVER]: %s has left the chat.\n", time_str, username);

    close(client_socket);
    return NULL;
}


// Broadcast message to all clients
void broadcast_message(const char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].socket != sender_socket) {
            if (send(clients[i].socket, message, strlen(message), 0) == -1) {
                perror("Error sending message");
                continue;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    }
}

//remove client from the list
void remove_client(int socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == socket) {
            clients[i].socket = 0;
            memset(clients[i].username, 0, sizeof(clients[i].username));
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


// Check if a username is already taken
int is_username_taken(const char *username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && strcmp(clients[i].username, username) == 0) {
            return 1;
        }
    }
    return 0;
}

