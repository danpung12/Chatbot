#include "message_handler.h"
#include <stdio.h>
#include <unistd.h>
#include <strings.h>

void *read_messages(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[1024];
    while (1) {
        bzero(buffer, 1024);
        if (read(sockfd, buffer, 1023) == 0) {
            break;
        }
        printf("%s", buffer);
    }
    return NULL;
}
