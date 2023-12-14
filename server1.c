#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <time.h>

#define MAX_BUFFER 1024
#define MAX_MESSAGE 2048
#define MAX_CLIENTS 30

typedef struct {
    int socket;
    char nickname[32];
} Client;

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

void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[MAX_BUFFER];
    FILE *file = NULL;  // 파일 전송을 위한 변수 추가

    while (1) {
        bzero(buffer, MAX_BUFFER);
        if (read(client->socket, buffer, MAX_BUFFER - 1) == 0) {
            break;
        }

        if (strncmp(buffer, "FILE_TRANSMIT_START:", 20) == 0) {
            if (file != NULL) {
                fclose(file);
                file = NULL;
            }

            // 파일명 추출
            char *file_name = strtok(buffer + 20, ":");

            // 파일 생성 및 열기
            file = fopen(file_name, "wb");
            if (file == NULL) {
                printf("파일을 열 수 없습니다.");
                continue;
            }

            printf("파일 '%s' 수신을 시작합니다.", file_name);
        } else if (strncmp(buffer, "FILE_TRANSMIT_END", 17) == 0) {
            // 파일 닫기
            if (file != NULL) {
                fclose(file);
                file = NULL;
                printf("파일 수신을 완료했습니다.");
            }
        } else if (file != NULL) {
            // 파일 쓰기
            fwrite(buffer, 1, strlen(buffer), file);
        } else {
            // 기존 메시지 처리 로직 유지
        }
    }

    close(client->socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == client->socket) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}


int main(int argc, char *argv[]) {

    int sockfd, newsockfd, port_no;

    struct sockaddr_in serv_addr, cli_addr;

    socklen_t clilen;



    if (argc < 2) {

        fprintf(stderr, "ERROR, no port provided");

        exit(1);

    }



    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {

        perror("ERROR opening socket");

        exit(1);

    }



    bzero((char *)&serv_addr, sizeof(serv_addr));

    port_no = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_addr.s_addr = INADDR_ANY;

    serv_addr.sin_port = htons(port_no);



    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {

        perror("ERROR on binding");

        exit(1);

    }



    listen(sockfd, 5);

    clilen = sizeof(cli_addr);



    while (1) {

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);



        if (newsockfd < 0) {

            perror("ERROR on accept");

            exit(1);

        }



        pthread_mutex_lock(&clients_mutex);

        if (client_count >= MAX_CLIENTS) {

            printf("클라이언트 수가 너무 많습니다.");

            close(newsockfd);

        } else {

            clients[client_count].socket = newsockfd;



            bzero(clients[client_count].nickname, 32);

            read(newsockfd, clients[client_count].nickname, 31);



            printf("%s가 연결되었습니다.", clients[client_count].nickname);



            client_count++;



            pthread_t thread;

            pthread_create(&thread, NULL, handle_client, (void *)&clients[client_count - 1]);

            pthread_detach(thread);

        }

        pthread_mutex_unlock(&clients_mutex);

    }



    close(sockfd); // 소켓 닫기

    return 0;

}
