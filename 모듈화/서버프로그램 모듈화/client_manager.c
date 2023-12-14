#include "client_manager.h"
#include <string.h>

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int find_client_by_nickname(const char *nickname) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].nickname, nickname) == 0) {
            return i;
        }
    }
    return -1;  // 찾지 못한 경우
}
