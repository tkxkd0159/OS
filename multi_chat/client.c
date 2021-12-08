#include <stdio.h>
#include <unistd.h>  // close
#include <stdlib.h>  // EXIT_FAILURE, free
#include <string.h>  // strlen, memset
#include <pthread.h> // pthread_t
#include <signal.h>  // signal, SIGINT

#include <sys/socket.h> // PF_INET, AF_INET, SOCK_STREAM
                        // socket, sockaddr, connect, send
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr

#define CHAT_LEN 2048
#define BUF_SIZE CHAT_LEN+64
volatile sig_atomic_t FLAG = 0; // 시그널 핸들러를 위한 전역변수

void str_trim_lf(char* arr, int len);
void chat_prefix();
void sigint_handler();
void send_msg_handler(void* sockfd);
void recv_msg_handler(void* sockfd);


int main() {
    signal(SIGINT, sigint_handler);

    char addr[20];
    int port;
    printf("Input Server IP Address : ");
    scanf("%s", addr);
    printf("Input Server Port Number : ");
    getchar();
    scanf("%d", &port);
    printf("open!! client\n");

    struct sockaddr_in serv_addr;
    // server.c에서 설정해줬던 소켓과 동일하게 설정
    int sockfd = socket(PF_INET, SOCK_STREAM, 0); // IPv4 프로토콜 연결 지향형 소켓 생성 (TCP)
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addr);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("ERROR : connection failed\n");
        return EXIT_FAILURE;
    } else {
        printf("Success to connect to server!\n");
        printf("Chatting on...\n");
    }

    char msg[CHAT_LEN] = "";
    int recv_bytes = recv(sockfd, msg, CHAT_LEN, 0);
    if (recv_bytes > 0){
        printf("%s\n", msg);
    }
    memset(msg, 0, CHAT_LEN);
    printf("input 'Q' to exit\n");

    pthread_t send_msg_thread;
    pthread_t recv_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, (void*)&sockfd) != 0){
        printf("ERROR : pthread can't be generated\n");
        return EXIT_FAILURE;
    }
    if (pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, (void*)&sockfd) != 0){
        printf("ERROR : pthread can't be generated\n");
        return EXIT_FAILURE;
    }


    while(1){
        if(FLAG){
            printf("\nLeave chat room\n");
            break;
        }
    }

    close(sockfd);



    return 0;
}

void str_trim_lf(char* arr, int len){
    int i;
    for (i = 0; i < len; i++){
        if (arr[i] == '\n'){
            arr[i] = '\0';
            break;
        }
    }
}

void chat_prefix(){
    printf("%s", "me : ");
    fflush(stdout);
}

void sigint_handler(){
    FLAG = 1;
}

void send_msg_handler(void* arg){
    int sockfd = *((int*) arg);
    char msg[CHAT_LEN] = "";
    char buffer[BUF_SIZE] = "";
    int start_sig = 1;

    while(1){
        fgets(msg, CHAT_LEN, stdin);
        str_trim_lf(msg, CHAT_LEN);
        if (strcmp(msg, "Q") == 0){
            break;
        }
        else {
            sprintf(buffer, "%s", msg);
            send(sockfd, buffer, strlen(buffer), 0);
            chat_prefix();
        }
        memset(msg, 0, CHAT_LEN);
        memset(buffer, 0, BUF_SIZE);
    }
    // Q 입력 시 종료를 위한 핸들러 호출
    sigint_handler();
}

void recv_msg_handler(void* arg){
    int sockfd = *((int*) arg);
    char msg[CHAT_LEN] = "";
    while(1){
        int recv_bytes = recv(sockfd, msg, CHAT_LEN, 0);
        if (recv_bytes > 0){
            printf("%s", msg);
            chat_prefix();
        }
        else if (recv_bytes == 0){
            break;
        }
        else {
            printf("ERROR : sockfd read failed\n");
        }
        memset(msg, 0, CHAT_LEN);
    }
}