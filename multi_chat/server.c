#include <stdio.h>
#include <unistd.h>  // socklen_t
#include <stdlib.h>  // EXIT_FAILURE, atoi
#include <pthread.h> // pthread_t
#include <signal.h>  // signal, SIGPIPE, SIG_IGN

#include <sys/socket.h> // AF_INET, SOCK_STREAM, SO_REUSEPORT, SO_REUSEADDR
                        // setsockopt, socket, sockaddr, bind, listen, accpet
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr

#define MAX_CLIENTS 100
static _Atomic unsigned int cli_cnt = 0;  // 여러 스레드에서 클라이언트 생성 후 셀 때 정확히 변경되도록 _Atomic 선언
static int uid = 10;

// client structure
typedef struct{
    struct sockaddr_in addr;
    int sockfd;
    int uid;
    char name[32];
} client_t;

client_t* clients[MAX_CLIENTS];

int main(int argc, char* argv[]) {
    const char *s_ip = "127.0.0.1";
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int s_port = atoi(argv[1]);
    int serv_sock, cli_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    // socket setting
    serv_sock = socket(PF_INET, SOCK_STREAM, 0); // IPv4 프로토콜 연결 지향형 소켓 생성 (TCP)
    if (serv_sock == -1) {
        printf("ERROR : socket creation\n");
        return EXIT_FAILURE;
    }
    serv_addr.sin_family = AF_INET; // IPv4 주소체계 설정
    serv_addr.sin_addr.s_addr = inet_addr(s_ip); // 서버 호스트 주소 설정
    serv_addr.sin_port = htons(s_port);          // 서버 포트 설정
    signal(SIGPIPE, SIG_IGN); // 연결이 끊어진 소켓에 데이터를 쓰려고 할 때 SIGPIPE 발생되면서 서버 다운되는 것 방지

    // 서버 종료 시 다시 실행하면 커널에서 설정된 TIME_WAIT가 경과할 때까지 소켓이 살아있어서 bind 불가
    // 따라서 재시작 시 바로 다시 할당하도록 소켓 옵션 변경
    int option = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &option, sizeof(option)) < 0) {
        printf("ERROR : socket's option setting");
        return EXIT_FAILURE;
    }

    // 소켓에 주소와 포트를 지정
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR : socket binding");
        return EXIT_FAILURE;
    }

    // 소켓을 연결 요청 대기 상태로 바꿈. 연결 요청 큐 크기는 임의로 10으로 설정
    if (listen(serv_sock, 10) < 0) {
        printf("ERROR : socket listening");
        return EXIT_FAILURE;
    }

    // 대기하다가 연결 요청이 오면 수락
    while(1) {
        socklen_t cliaddr_len = sizeof(cli_addr);
        cli_sock = accept(serv_sock, (struct sockaddr*)&cli_addr, &cliaddr_len);
    }

    printf("Server Port : %d \n", s_port);
    printf("Open!! server\n");
    printf("Chatting on\n");
}