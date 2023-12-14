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
char nickname[32]; // 닉네임도 전역 변수로 변경

// 현재 시간을 문자열로 변환하는 함수
void get_time_string(char *buffer, int size) {
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, size, "(%H:%M)", timeinfo);
}

// 서버로부터 메시지를 읽어 출력하는 함수
void *read_messages(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[MAX_BUFFER];

    while (1) {
        bzero(buffer, MAX_BUFFER);
        if (read(sockfd, buffer, MAX_BUFFER - 1) == 0) {
            break;
        }

        // 파일 전송 요청 메시지 처리
        if (strncmp(buffer, "FILE_TRANSMIT_REQUEST", 20) == 0) {
            // 파일명 추출
            char *file_name = strtok(buffer + 20, ":");

            // 사용자에게 수락 여부 묻기
            char response;
            printf("%s님이 %s 파일을 보내려고 합니다. 수락하시겠습니까? y/n: ", nickname, file_name);
            scanf(" %c", &response);
            getchar(); // 버퍼 비우기

            if (response == 'y' || response == 'Y') {
                // 수락 메시지 전송
                char message[MAX_MESSAGE];
                snprintf(message, sizeof(message), "FILE_TRANSMIT_ACCEPT:%s:%s", nickname, file_name);
                write(sockfd, message, strlen(message));
            } else if (response == 'n' || response == 'N') {
                // 거절 메시지 전송
                char message[MAX_MESSAGE];
                snprintf(message, sizeof(message), "FILE_TRANSMIT_REJECT:%s:%s", nickname, file_name);
                write(sockfd, message, strlen(message));
            }
        } else {
            printf("%s", buffer); // 서버로부터 받은 메시지 출력
        }
    }
    return NULL;
}

// 시그널 핸들러 함수
void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("======= 메뉴 =======");
        printf("1. 채팅 나가기");
        printf("2. 메시지 전송");
        printf("3. 파일 전송");
        printf("0. 메뉴 종료");
        printf("===================");

        char option;
        printf("메뉴를 선택하세요: ");
        scanf(" %c", &option);
        getchar(); // 버퍼 비우기

        if (option == '1') {
            printf("서버 연결 종료");
            close(sockfd);
            exit(0);
        } else if (option == '2') {
            // 메시지 작성 및 전송
            char buffer[MAX_BUFFER];
            printf("메시지를 입력하세요: ");
            fgets(buffer, MAX_BUFFER - 1, stdin);
            buffer[strcspn(buffer, "")] = 0; // 개행 문자 제거
            write(sockfd, buffer, strlen(buffer));
        } else if (option == '3') {
            // 파일 전송 요청
            char receiver_nickname[32];
            char file_name[32];
            printf("파일을 전송할 사용자의 닉네임을 입력하세요: ");
            fgets(receiver_nickname, 31, stdin);
            receiver_nickname[strcspn(receiver_nickname, "")] = 0; // 개행 문자 제거

            printf("전송할 파일명을 입력하세요: ");
            fgets(file_name, 31, stdin);
            file_name[strcspn(file_name, "")] = 0; // 개행 문자 제거

            char message[MAX_MESSAGE];
            snprintf(message, sizeof(message), "FILE_TRANSMIT_REQUEST:%s:%s", receiver_nickname, file_name);
            write(sockfd, message, strlen(message)); 
        } else if (option == '0') {
            printf("채팅으로 돌아갑니다.");
        } else {
            printf("없는 메뉴 번호입니다.");
        }
    }
}

int main(int argc, char *argv[]) {
    int port_no;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // 사용자로부터 호스트 이름과 포트 번호를 입력받음
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

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    // 닉네임 전송
    printf("사용할 닉네임: ");
    fgets(nickname, 31, stdin);
    nickname[strcspn(nickname, "")] = 0; // 개행 문자 제거
    write(sockfd, nickname, strlen(nickname)); // 닉네임 서버에 전송

    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);

    // 별도의 스레드에서 메시지 읽기 시작
    pthread_t read_thread;
    pthread_create(&read_thread, NULL, read_messages, (void *)&sockfd);
    pthread_detach(read_thread);

    // 메인 스레드는 메시지 작성에 사용됨
    char buffer[MAX_BUFFER];
    while (1) {
        fgets(buffer, MAX_BUFFER - 1, stdin);
        buffer[strcspn(buffer, "")] = 0; // 개행 문자 제거

        char time_string[10];
        get_time_string(time_string, sizeof(time_string)); // 현재 시간 가져오기

        char message[MAX_MESSAGE];
        snprintf(message, MAX_MESSAGE, "%s [%s] :%s", time_string, nickname, buffer); // 메시지 작성

        // 클라이언트 측에서 메시지 출력
        write(sockfd, message, strlen(message)); // 메시지 서버에 전송
    }

    return 0;
}
