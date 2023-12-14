#include "signal_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

extern int sockfd;  // main.c에서 정의된 전역 변수

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
