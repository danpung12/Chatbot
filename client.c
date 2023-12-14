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

// 현재 시간을 문자열로 변환하는 함수

void get_time_string(char *buffer, int size) {
    time_t rawtime;
    struct tm *timeinfo;
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
        // 파일 수신 외의 메시지는 그대로 출력
        printf("%s\n", buffer);
    }
    return NULL;
}

// 시그널 핸들러 함수
void signal_handler(int signal) {

    if (signal == SIGINT) {

        printf("\n======= 메뉴 =======\n");
        printf("1. 채팅 나가기\n");
        printf("2. 메시지 전송\n");
        printf("3. 파일 전송\n");
        printf("0. 메뉴 종료\n");
        printf("===================\n");

        char option;

        printf("메뉴를 선택하세요: ");
        scanf(" %c", &option);
        getchar(); // 버퍼 비우기

        if (option == '1') {
            printf("서버 연결 종료\n");
            close(sockfd);
            exit(0);

        } else if (option == '2') {

            // 메시지 작성 및 전송

            char buffer[MAX_BUFFER];

            printf("메시지를 입력하세요: ");
            fgets(buffer, MAX_BUFFER - 1, stdin);
            buffer[strcspn(buffer, "\n")] = 0; // 개행 문자 제거
            write(sockfd, buffer, strlen(buffer));

        } else if (option == '3') {

            // 파일 전송 요청
            char receiver_nickname[32];
            char file_name[32];
            
            printf("파일을 전송할 사용자의 닉네임을 입력하세요: ");
            fgets(receiver_nickname, 31, stdin);

            receiver_nickname[strcspn(receiver_nickname, "\n")] = 0; // 개행 문자 제거
            printf("전송할 파일명을 입력하세요: ");
            fgets(file_name, 31, stdin);
            
            file_name[strcspn(file_name, "\n")] = 0; // 개행 문자 제거
            char message[MAX_MESSAGE];
            snprintf(message, sizeof(message), "FILE_TRANSMIT_START:%s:%s", receiver_nickname, file_name);
            write(sockfd, message, strlen(message));

            // 파일 열기
            FILE *file = fopen(file_name, "rb");
            if (file == NULL) {
                printf("파일을 열 수 없습니다.\n");
                return;
            }
            // 파일 내용을 읽어서 서버에 전송
            char buffer[MAX_BUFFER];
            while (1) {
                size_t bytes_read = fread(buffer, 1, MAX_BUFFER - 1, file);
                if (bytes_read <= 0) {
                    break;  // 파일 내용이 더 이상 없으면 종료
                }
                write(sockfd, buffer, bytes_read);  // 읽은 내용을 서버에 전송

            }
            // 파일 전송 종료 신호 전송
            write(sockfd, "FILE_TRANSMIT_END", 17);

            // 파일 닫기

            fclose(file);
        } else if (option == '0') {
            printf("채팅으로 돌아갑니다.\n");

        } else {
            printf("없는 메뉴 번호입니다.\n");

        }
    }
}

int main(int argc, char *argv[]) {
    int port_no;

    struct sockaddr_in serv_addr;

    struct hostent *server;
    // 사용자로부터 호스트 이름과 포트 번호를 입력받음

    if (argc < 3) {

        fprintf(stderr, "usage %s hostname port\n", argv[0]);
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
        fprintf(stderr, "ERROR, no such host\n");
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
    char nickname[32];
    
    printf("사용할 닉네임: ");
    fgets(nickname, 31, stdin);
    nickname[strcspn(nickname, "\n")] = 0; // 개행 문자 제거
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
        buffer[strcspn(buffer, "\n")] = 0; // 개행 문자 제거
        char time_string[10];
        get_time_string(time_string, sizeof(time_string)); // 현재 시간 가져오기
        char message[MAX_MESSAGE];
        snprintf(message, MAX_MESSAGE, "%s [%s]: %s", time_string, nickname, buffer); // 메시지 작성

 // 귓속말 메시지 처리
if (strncmp(buffer, "/m ", 3) == 0) {
    write(sockfd, buffer, strlen(buffer));
    continue;

}
        // 클라이언트 측에서 메시지 출력
        printf("%s\n", message);
        
        // 메시지를 서버에 전송
        write(sockfd, message, strlen(message));

    }

    close(sockfd); // 소켓 닫기
    return 0;

}
