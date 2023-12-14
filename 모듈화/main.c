//gcc -o chat_client main.c time_handler.c message_handler.c signal_handler.c

#include "time_handler.h"

#include "message_handler.h"

#include "signal_handler.h"

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



#define MAX_BUFFER 1024

#define MAX_MESSAGE (MAX_BUFFER + 64) // 시간과 닉네임을 위한 추가 공간



int sockfd;  // 클라이언트 소켓. 전역 변수로 변경



int main(int argc, char *argv[]) {

    int port_no;

    struct sockaddr_in serv_addr;



    struct hostent *server;

	

    if (argc < 3) {

        fprintf(stderr, "usage %s hostname port", argv[0]);

        exit(1);

    }



    port_no = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);



    if (sockfd < 0) {

        perror("ERROR opening socket");

        exit(1);

    }



    server = gethostbyname(argv[1]);



    if (server == NULL) {

        fprintf(stderr, "ERROR, no such host");

        exit(1);

    }



    

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(port_no);



    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {

        perror("ERROR connecting");

        exit(1);

    }



    char nickname[32];

    

    printf("사용할 닉네임: ");

    fgets(nickname, 31, stdin);

    nickname[strcspn(nickname, "\n")] = 0; // 개행 문자 제거

    write(sockfd, nickname, strlen(nickname)); // 닉네임 서버에 전송



    signal(SIGINT, signal_handler);

    pthread_t read_thread;

    pthread_create(&read_thread, NULL, read_messages, (void *)&sockfd);

    pthread_detach(read_thread);



    char buffer[MAX_BUFFER];

    while (1) {

        fgets(buffer, MAX_BUFFER - 1, stdin);

        buffer[strcspn(buffer, "\n")] = 0; // 개행 문자 제거

        char time_string[10];

        get_time_string(time_string, sizeof(time_string)); // 현재 시간 가져오기

        char message[MAX_MESSAGE];

        snprintf(message, MAX_MESSAGE, "%s [%s]: %s", time_string, nickname, buffer); // 메시지 작성



        if (strncmp(buffer, "/m ", 3) == 0) {

            write(sockfd, buffer, strlen(buffer));

            continue;

        }



        printf("%s\n", message); // 클라이언트 측에서 메시지 출력

        write(sockfd, message, strlen(message)); // 메시지를 서버에 전송

    }



    close(sockfd); // 소켓 닫기

    return 0;

}
