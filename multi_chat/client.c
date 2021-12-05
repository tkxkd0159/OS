#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

int main() {
    char addr[20];
    int port;
    printf("Input Server IP Address : ");
    scanf("%s", addr);
    printf("Input Server Port Number : ");
    getchar();
    scanf("%d", &port);
    printf("addr size is %lu\n", sizeof(addr) / sizeof(char));
    printf("Port Number is : %d\n", port);


    // printf("Input 'Q' to exit \n");
    // char trigger;
    // while (1) {
    //     scanf("%c", &trigger);

    //     if (trigger == 'Q') {
    //         break;
    //     }
    //     else {
    //         printf("%c \n", trigger);
    //     }
    // }
    return 0;
}