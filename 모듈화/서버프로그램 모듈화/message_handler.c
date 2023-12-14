#include "message_handler.h"
#include "client_manager.h"
#include <stdio.h>
#include <unistd.h>
#include <strings.h>

void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[MAX_BUFFER];

    while (1) {
        int bytes_read = read(client->socket, buffer, sizeof(buffer) - 1);

        if (bytes_read <= 0) {
            // 연결 종료 메시지
            char disconnect_message[MAX_BUFFER];
            snprintf(disconnect_message, sizeof(disconnect_message), "              %s님이 접속을 종료하셨습니다.", client->nickname);
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < client_count; i++) {
                if (clients[i].socket != client->socket) {
                    write(clients[i].socket, disconnect_message, strlen(disconnect_message));
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            return NULL;
        }

        buffer[bytes_read] = '\0';

        // 귓속말 메시지 처리
        char *recipient_nickname = strtok(buffer + 3, " ");
        char *message_content = strtok(NULL, "");

        if (recipient_nickname == NULL || message_content == NULL) {
            continue;
        }

        int recipient_index = find_client_by_nickname(recipient_nickname);

        if (recipient_index != -1) {
            char message[MAX_MESSAGE];
            snprintf(message, sizeof(message), "(귓속말) %s->%s: %s", client->nickname, recipient_nickname, message_content);
            write(clients[recipient_index].socket, message, strlen(message));
        } else {
            // 해당 사용자가 없는 경우
            char message[MAX_MESSAGE];
            snprintf(message, sizeof(message), "사용자 '%s'를 찾을 수 없습니다.", recipient_nickname);
            write(client->socket, message, strlen(message));
        }

        continue;  // 귓속말 메시지를 처리한 후에는 다음 메시지를 받도록 continue를 사용합니다.
    }
}
