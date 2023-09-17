#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> 
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include "server_helper.h"
void handle_stdin_msg(int tcpsocket){
    char *input = calloc(BUFSIZ, sizeof(char));
    fgets(input, BUFSIZ, stdin);
    input[strlen(input) - 1] = 0;
    if(strcmp(input, "exit") == 0) {
        char exit_msg[5] = "exit";
        int res = send(tcpsocket, exit_msg, strlen(exit_msg), 0);
        if(res < 0 ) {
            perror("Error exiting");
            exit(0);
        }
        close(tcpsocket);
        exit(0);
    }
    else if(strncmp(input, "subscribe", strlen("subscribe")) == 0 || strncmp(input, "unsubscribe", strlen("unsubscribe")) == 0){
        int res = send(tcpsocket, input, strlen(input), 0);
        if (res < 0) {
            perror("Error sending subscribe/unsubscribe");
            exit(0);
        }
        memset(input, 0, BUFSIZ);
        res = recv(tcpsocket, input, BUFSIZ, 0);
        int good_subscription = *((int*)input);
        char *msg = input + 4;
        if(good_subscription)
            printf("%s\n", msg);
    }
    else return;
}
void handle_received_msg(int tcpsocket){
    struct msg received_msg;
    int res = recv(tcpsocket, &received_msg, sizeof(struct msg), 0);
    if(res < 0) {
        perror("Error receiving message");
        exit(0);
    }
    if(strncmp(received_msg.buf, "exit", 4) == 0) {
        exit(0);
    }
    char buf[1554];
    memcpy(buf, received_msg.buf, 1552);
    char *topic = calloc(50, sizeof(char));
    memcpy(topic, buf, 50);
    uint8_t type = *((uint8_t*) (buf+50));
    char *payload = calloc(1501, sizeof(char));
    memcpy(payload, buf+51, 1500);
    if(type == 0) {
        uint8_t sign = payload[0];
        uint32_t number;
        memcpy(&number, payload + 1, 4);
        number = ntohl(number);
        if(sign == 1)
            number = (-1) * number;
        printf("%s:%d - %s - INT - %d\n", received_msg.ip, received_msg.port, topic, number);
        return;
    }
    else if(type == 1) {
        uint16_t number;
        memcpy(&number, payload, 2);
        number = ntohs(number);
        printf("%s:%d - %s - SHORT_REAL - %.2f\n", received_msg.ip, received_msg.port, topic, (float)number / 100);
        return;
    }
    else if(type == 2) {
        uint8_t sign = payload[0];
        int number;
        memcpy(&number, payload + 1, 4);
        number = ntohl(number);
        uint8_t modul = payload[5];
        if(sign == 1)
            number *= (-1);
        int power = 1;
        for(int i = 0; i < modul ; i++)
            power *= 10;
        printf("%s:%d - %s - FLOAT - %.4f\n",received_msg.ip, received_msg.port, topic, (float)number / (float)power);
        return;
    }
    else if(type == 3) {
        printf("%s:%d - %s - STRING - %s\n", received_msg.ip, received_msg.port, topic, payload);
    }

}
int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    struct sockaddr_in server_addr;
    if(argc != 4) {
        perror("Error at arguments, usage: ./server <ID> <IP Server> <PORT>");
        exit(0);
    }
    int tcpsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(tcpsocket < 0) {
        perror("Error creating tcp socket");
        exit(0);
    }
    int flag = 1;
    int res = setsockopt(tcpsocket, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int));
    if(res < 0) {
        perror("Error in setsockopt");
        exit(0);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], &server_addr.sin_addr);
    res = connect(tcpsocket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(res < 0) {
        perror("Error binding tcp socket");
        exit(0);
    }
    res = send(tcpsocket, argv[1], strlen(argv[1]) + 1, 0);
    if(res < 0) {
        perror("Eroare la send ID");
        exit(0);
    }
    fd_set file_descriptors;
    FD_ZERO(&file_descriptors);
    FD_SET(STDIN_FILENO, &file_descriptors);
    FD_SET(tcpsocket, &file_descriptors);
    int no_filedes = tcpsocket;
    while(1) {
        fd_set old_filedescriptors;
        FD_ZERO(&old_filedescriptors);
        old_filedescriptors = file_descriptors;
        res = select(no_filedes + 1, &old_filedescriptors, NULL, NULL, NULL);
        if(res < 0) {
            perror("Error at select");
            exit(0);
        }
        for(int i = 0; i <= no_filedes; i++) {
            if(FD_ISSET(i, &old_filedescriptors)) {
            if(i == 0) {
                handle_stdin_msg(tcpsocket);
                break;
            }
            else {
                handle_received_msg(tcpsocket);
                break;
            }
        }
        }
    }
}