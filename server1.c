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

    FILE *fp = NULL;  // 파일 포인터



    while (1) {

        bzero(buffer, MAX_BUFFER);

        if (read(client->socket, buffer, MAX_BUFFER - 1) == 0) {

            break;

        }



        if (strncmp(buffer, "FILE_TRANSMIT_START:", 20) == 0) {

            char *receiver_nickname = strtok(buffer + 20, ":");

            char *file_name = strtok(NULL, ":");



            int receiver_index = find_client_by_nickname(receiver_nickname);

            if (receiver_index != -1) {

                char message[MAX_MESSAGE];

                snprintf(message, sizeof(message), "%s님이 %s 파일을 보내려고 합니다. 수락하시겠습니까? y/n", client->nickname, file_name);

                write(clients[receiver_index].socket, message, strlen(message));

            }

        } else if (strncmp(buffer, "FILE_TRANSMIT_ACCEPT:", 21) == 0) {

            char *sender_nickname = strtok(buffer + 21, ":");

            char *file_name = strtok(NULL, ":");



            int sender_index = find_client_by_nickname(sender_nickname);

            if (sender_index != -1) {

                char file_path[512];

                snprintf(file_path, sizeof(file_path), "%s%s", "/tmp/", file_name);  // 임시 파일 경로

                fp = fopen(file_path, "wb");  // 파일 생성

                if (fp == NULL) {

                    perror("파일 생성 실패");

                    continue;

                }

                

                // 파일 데이터를 다른 클라이언트에게 전송하는 로직

                for (int i = 0; i < client_count; i++) {

                    if (clients[i].socket != client->socket) {

                        write(clients[i].socket, buffer, strlen(buffer));

                    }

                }

            }

        } else if (strncmp(buffer, "FILE_TRANSMIT_END:", 18) == 0) {

            if (fp != NULL) {

                fclose(fp);  // 파일 닫기

                fp = NULL;

            }

        } else if (fp != NULL) {

            fwrite(buffer, sizeof(char), strlen(buffer), fp);  // 파일에 데이터 쓰기

        } else {

            time_t rawtime;

            struct tm * timeinfo;

            char timestamp[64];

            time(&rawtime);

            timeinfo = localtime(&rawtime);

            strftime(timestamp, sizeof(timestamp), "(%H:%M)", timeinfo);



            char message[MAX_MESSAGE];

            snprintf(message, sizeof(message), "%s [%s]: %s", timestamp, client->nickname, buffer);



            pthread_mutex_lock(&clients_mutex);

            for (int i = 0; i < client_count; i++) {

                if (clients[i].socket != client->socket) {

                    write(clients[i].socket, message, strlen(message));

                }

            }

            pthread_mutex_unlock(&clients_mutex);

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
