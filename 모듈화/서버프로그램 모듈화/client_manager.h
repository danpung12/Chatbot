#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <pthread.h>

#define MAX_CLIENTS 30
#define MAX_BUFFER 1024

typedef struct {
    int socket;
    char nickname[32];
} Client;

extern Client clients[MAX_CLIENTS];
extern int client_count;
extern pthread_mutex_t clients_mutex;

int find_client_by_nickname(const char *nickname);

#endif
