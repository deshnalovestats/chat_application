#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

// Constants
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define USERNAME_SIZE 100
#define MSG_SIZE 2048

// Function prototypes
void handle_client_message(int client_socket, char *message);
void broadcast_message(const char *message, int sender_socket);

#endif // COMMON_H
