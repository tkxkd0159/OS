#include <stdio.h>
#include <unistd.h>  // socklen_t, close, write, sleep
#include <stdlib.h>  // EXIT_FAILURE, free
#include <string.h>  // strlen, memset
#include <pthread.h> // pthread_t, pthread_mutex_t, pthread_detach, pthread_self
#include <signal.h>  // signal, SIGPIPE, SIG_IGN

#include <sys/socket.h> // PF_INET, AF_INET, SOCK_STREAM, SO_REUSEPORT, SO_REUSEADDR
                        // setsockopt, socket, sockaddr, bind, listen, accept, recv
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_addr

#define MAX_CLIENTS 100
#define BUF_SIZE 2048

static _Atomic unsigned int cli_cnt = 0;  // 여러 스레드에서 클라이언트 생성 후 셀 때 순차적 변경되도록 _Atomic 선언
static int uids[100] = {};  // 내가 제한해놓은 인원에서 임의의 uid를 부여하기 위한 배열

// client structure
typedef struct{
    struct sockaddr_in addr;
    int sockfd;
    int uid;
} client_t;

client_t* clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // 연결된 클라이언트 처리 시 각 액션의 독점 처리 보장을 위한 뮤텍스 설정
void queue_add(client_t *cl);
void queue_remove(int uid);

void str_trim_lf(char arr[], int len);
void print_ip_addr(struct sockaddr_in addr);
void flood_msg(char *msg, int from_uid);
void send_msg(char *msg, client_t *cl);
void* handle_client(void* arg);
int choose_uid();

int main(int argc, char* argv[]) {
    const char *s_ip = "127.0.0.1";
    // if (argc != 2) {
    //     printf("Usage: %s <port>\n", argv[0]);
    //     return EXIT_FAILURE;
    // }
    // int s_port = atoi(argv[1]);
    int s_port;
    printf("Server Port : ");
    scanf("%d", &s_port);
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
        printf("ERROR : socket's option setting failed\n");
        return EXIT_FAILURE;
    }

    // 소켓에 주소와 포트를 지정
    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR : socket binding failed\n");
        return EXIT_FAILURE;
    }

    // 소켓을 연결 요청 대기 상태로 바꿈. 연결 요청 큐 크기는 임의로 10으로 설정
    if (listen(serv_sock, 10) < 0) {
        printf("ERROR : socket listening failed\n");
        return EXIT_FAILURE;
    }
    printf("Open!! server\n");
    printf("Chatting on\n");

    // 대기하다가 연결 요청이 오면 수락
    while(1) {
        socklen_t cliaddr_len = sizeof(cli_addr);
        cli_sock = accept(serv_sock, (struct sockaddr*)&cli_addr, &cliaddr_len);

        if (cli_cnt == MAX_CLIENTS) {
            printf("ERROR : Maximum clients connected\n");
            print_ip_addr(cli_addr);
            char* reject_msg = "Due to the server capacity limitation, you were disconnected after connection";
            write(cli_sock, reject_msg, strlen(reject_msg));
            close(cli_sock);
            continue;
        }

        client_t *cli = (client_t*)malloc(sizeof(client_t)); // 런타임에서 클라이언트에 대한 메모리 할당
        cli->addr = cli_addr;
        cli->sockfd = cli_sock;
        cli->uid = choose_uid();
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        sleep(1);

    }


}


void str_trim_lf(char* arr, int len) {
    for (int i=0; i<len; i++){
        if(arr[i] == '\n'){
            arr[i] = '\0';
            break;
        }
    }
}

void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);
    for (int i=0; i<MAX_CLIENTS; i++){
        if(!clients[i]){
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

}

void queue_remove(int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i=0; i<MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid == uid){
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);

}


// 리틀엔디안으로 저장된 주소값을 시프트연산자를 통해 처리 후 출력
void print_ip_addr(struct sockaddr_in addr) {
    printf("SRC : %d.%d.%d.%d\n",
            addr.sin_addr.s_addr & 0xff,
            (addr.sin_addr.s_addr & 0xff00) >> 8,
            (addr.sin_addr.s_addr & 0xff0000) >> 16,
            (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// sender를 제외한 모든 연결된 클라이언트에게 메세지 전송
void flood_msg(char* msg, int from_uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i=0; i<MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid != from_uid){
                if(write(clients[i]->sockfd, msg, strlen(msg)) < 0){
                printf("ERROR : Write to descriptor failed\n");
                break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// 특정 클라이언트에게 메세지 전송
void send_msg(char* msg, client_t *cl){
    pthread_mutex_lock(&clients_mutex);
    if (write(cl->sockfd, msg, strlen(msg)) < 0){
        printf("ERROR : write to descriptor failed\n");
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* arg){
    char buffer[BUF_SIZE];
    char flood_buffer[BUF_SIZE+16];
    int exit_flag = 0;
    cli_cnt++;

    client_t* cli = (client_t*)arg;
    printf("connected to Client %d\n", cli->uid);
    sprintf(buffer, "\rServer : Client %d has been connected\n", cli->uid);
    send_msg("Server : welcome to chatting server!", cli);
    flood_msg(buffer, cli->uid);
    memset(buffer, 0, BUF_SIZE);

    while(1){
        if (exit_flag){
            break;
        }

        int recv_bytes = recv(cli->sockfd, buffer, BUF_SIZE, 0);
        if (recv_bytes > 0){
            if(strlen(buffer) > 0){
                str_trim_lf(buffer, strlen(buffer));
                sprintf(flood_buffer, "\rClient %d : %s\n", cli->uid, buffer);
                flood_msg(flood_buffer, cli->uid);
                printf("message from client %d : %s\n", cli->uid, buffer);
            }
        }
        else if (recv_bytes == 0 || strcmp(buffer, "Q") == 0){
            sprintf(buffer, "\rServer : Client %d has left\n", cli->uid);
            printf("Client %d has left\n", cli->uid);
            flood_msg(buffer, cli->uid);
            exit_flag = 1;
        }
        else {
            printf("ERROR : Client handling failed\n");
            exit_flag = 1;
        }
        memset(buffer, 0, BUF_SIZE);
        memset(flood_buffer, 0, BUF_SIZE+16);
    }
    close(cli->sockfd);
    queue_remove(cli->uid);
    uids[cli->uid-1] = 0;
    free(cli);
    cli_cnt--;
    pthread_detach(pthread_self());

    return NULL;
}

int choose_uid(){
    for(int i=0; i < MAX_CLIENTS; i++){
        if (uids[i] == 0){
            uids[i] = 1;
            return i+1;
        }
    }
}
