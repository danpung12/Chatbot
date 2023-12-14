#include "network_config.h"
#include "client_manager.h"
#include "message_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>

#include "network_config.h"
#include "client_manager.h"
#include "message_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

void setup_server(int argc, char *argv[]) {
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
            read(newsockfd, clients[client_count].nickname, sizeof(clients[client_count].nickname) - 1);
            clients[client_count].nickname[sizeof(clients[client_count].nickname) - 1] = '\0';
            printf("%s가 연결되었습니다.\n", clients[client_count].nickname);
            client_count++;
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void *)&clients[client_count - 1]);
            pthread_detach(thread);
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    close(sockfd); // 소켓 닫기
}
