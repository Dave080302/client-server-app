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

int max(int a, int b) {
    if(a > b)
        return a;
    return b;
}
void handle_udp_message(int udpsocket, struct Client *clients, int clients_no){
    char topic[50];
    char buf[1554];
    struct sockaddr_in sender_socket;
    unsigned int len = sizeof(struct sockaddr_in);
    int res = recvfrom(udpsocket, buf, 1554, 0, (struct sockaddr*)&sender_socket, &len);
    if(res < 0) {
        perror("Error at receiving udp message");
        exit(0);
    }
    buf[res] = 0;
    memcpy(topic, buf, 50);
    for(int i = 0; i < clients_no; i++) {
        if(clients[i].connected == 1) {
            for(int j = 0; j < clients[i].no_topics; j++) {
                if(strcmp(clients[i].topics[j], topic) == 0) {
                    struct msg message;
                    memset(&message, 0, sizeof(struct msg));
                    strcpy(message.ip, inet_ntoa(sender_socket.sin_addr));
                    message.port = ntohs(sender_socket.sin_port);
                    memcpy(message.buf, buf, 1554);
                    res = send(clients[i].filedes, &message, sizeof(struct msg), 0);
                    if(res < 0) {
                        perror("Error sending data to client");
                        exit(0);
                    }
                }
            }
        }
        else {
            for(int j = 0; j < clients[i].no_topics; j++) {
                if(strcmp(clients[i].topics[j], topic) == 0 && clients[i].sfs[j] == 1) {
                    struct msg message;
                    memset(&message, 0, sizeof(struct msg));
                    strcpy(message.ip, inet_ntoa(sender_socket.sin_addr));
                    message.port = ntohs(sender_socket.sin_port);
                    memcpy(message.buf, buf, 1554);
                    memcpy(&clients[i].missed_messages[clients[i].no_bufs], &message, sizeof(struct msg));
                    if(clients[i].no_bufs >= 50) {
                        clients[i].missed_messages = realloc(clients[i].missed_messages, clients[i].no_bufs * sizeof(struct msg));
                    }
                    clients[i].no_bufs++;
                }
            }
        }
    }
}
void remove_from_vec(char** vec, int j, int* size, int* sfs) {
    for(int i = j; i < *size - 1; i++) {
        vec[i] = vec[i+1];
        sfs[i] = sfs[i+1];
    }
    *size = *size - 1;
}
void handle_client_message(int fd, struct Client* clients, int clients_no, int *no_fds, fd_set* fds){
    char *buf = calloc(BUFSIZ, sizeof(char));
    int res = recv(fd, buf, BUFSIZ, 0);
    if(res < 0) {
        perror("Error receiving client message");
        exit(0);
    }
    if(strncmp(buf, "exit", 4) == 0) {
        int i;
        for(i = 0; i < clients_no; i++)
            if(clients[i].filedes == fd)
                break;
        close(clients[i].filedes);
        clients[i].filedes = -1;
        clients[i].connected = 0;
        *no_fds = 4;
        for(int j = 0; j < clients_no; j++)
            *no_fds = max(*no_fds, clients[j].filedes);
        printf("Client %s disconnected.\n",clients[i].id);
        return;
    }
    if(strncmp(buf, "subscribe", strlen("subscribe")) == 0) {
        char *buf2 = strtok(buf, " ");
        buf2 = strtok(NULL, " "); /// topic
        char *buf1 = strtok(NULL, " "); /// sf
        int sf = atoi(buf1);
        int i;
        for(i = 0; i < clients_no; i++) {
            if(clients[i].filedes == fd)
                break;
        }
        int j;
        for(j = 0; j < clients[i].no_topics; j++)
            if(strcmp(buf2, clients[i].topics[j]) == 0) {
                struct ACK ack_msg;
                ack_msg.valid_subscription = 0;
                if(sf != clients[i].sfs[j])
                    clients[i].sfs[j] = sf;
                res = send(fd, (void*)&ack_msg, sizeof(ack_msg), 0); /// ACK
                if(res < 0) {
                    perror("Error sending acknowledge message");
                    exit(0);
                }
                return;
            }
        strcpy(clients[i].topics[clients[i].no_topics], buf2);
        clients[i].sfs[clients[i].no_topics] = sf;
        clients[i].no_topics++;
        struct ACK ack_msg;
        ack_msg.valid_subscription = 1;
        strcpy(ack_msg.msg, "Subscribed to topic.");
        res = send(fd, (void*)&ack_msg, sizeof(ack_msg), 0); /// ACK
        if(res < 0) {
            perror("Error sending acknowledge message");
            exit(0);
        }
    }
    if(strncmp(buf, "unsubscribe", strlen("unsubscribe")) == 0) {
        char *buf1 = strtok(buf, " ");
        buf1 = strtok(NULL, " ");
        int i;
        for(i = 0; i < clients_no; i++) {
            if(clients[i].filedes == fd)
                break;
        }
        int j;
        for(j = 0; j < clients[i].no_topics; j++)
            if(strcmp(buf1, clients[i].topics[j]) == 0) {
                remove_from_vec(clients[i].topics, j, &clients[i].no_topics, clients[i].sfs);
                struct ACK ack_msg;
                ack_msg.valid_subscription = 1;
                strcpy(ack_msg.msg, "Unsubscribed from topic.");
                res = send(fd, (void*)&ack_msg, sizeof(ack_msg), 0);
                if(res < 0) {
                    perror("Error sending stored message to client");
                    exit(0);
                }
                return;
        }
        struct ACK ack_msg;
        ack_msg.valid_subscription = 0;
        send(fd, (void*)&ack_msg, sizeof(ack_msg), 0);
        return;
    }
}
void handle_exit(int tcpsocket, int udpsocket, struct Client *clients, int clients_no){
    int res;
    char *buf = calloc(1552, sizeof(char));
    fgets(buf, 1552, stdin);
    if(strncmp(buf, "exit", 4) == 0) {
        for(int i = 0; i < clients_no; i++) {
            if(clients[i].connected == 1) {
                struct msg exit_msg;
                memset(exit_msg.ip , 0, 50);
                exit_msg.port = 0;
                strcpy(exit_msg.buf, "exit");
                res = send(clients[i].filedes, &exit_msg, sizeof(struct msg), 0);
                if(res < 0) {
                    perror("Error exiting");
                    exit(0);
                }
                close(clients[i].filedes);
            }
        }
        close(tcpsocket);
        close(udpsocket);
        exit(0);
    }
    return;
}
void handle_new_client(int tcpsocket, struct Client* clients, int *max_index, int *current_index, 
                        int* no_filedes, fd_set* file_descriptors, int port) {
    struct sockaddr_in new_client_addr;
    unsigned int len = sizeof(struct sockaddr_in);
    int client_socket = accept(tcpsocket, (struct sockaddr*)&new_client_addr, &len);
    if(client_socket < 0) {
        perror("Error creating client socket");
        exit(0);
    }
    char *id = calloc(30, sizeof(char));
    int res = recv(client_socket, id, 30, 0);
    if(res < 0) {
        perror("Error receiving client ID");
        exit(0);
    }
    for(int i = 0; i < *current_index; i++)
        if(clients[i].connected == 1 && strcmp(clients[i].id, id) == 0) {
            printf("Client %s already connected.\n", id);
            struct msg exit_msg;
            memset(exit_msg.ip , 0, 50);
            exit_msg.port = 0;
            strcpy(exit_msg.buf, "exit");
            int res = send(client_socket, &exit_msg, sizeof(struct msg), 0);
            if(res < 0) {
                perror("Error sending exit");
                exit(0);
            }
            close(client_socket);
            free(id);
            return;
        }
        else if(clients[i].connected == 0 && strcmp(clients[i].id, id) == 0) {
            clients[i].connected = 1;
            clients[i].filedes = client_socket;
            printf("New client %s connected from %s:%d.\n", id, inet_ntoa(new_client_addr.sin_addr), port);
            for(int j = 0; j < clients[i].no_bufs; j++) {
                send(clients[i].filedes, &clients[i].missed_messages[j], sizeof(struct msg),0);
                memset(&clients[i].missed_messages[j], 0, sizeof(struct msg));
            }
            clients[i].no_bufs = 0;
            FD_SET(clients[i].filedes, file_descriptors);
            *no_filedes = max(clients[i].filedes, *no_filedes);
            free(id);
            return;
        }
    if(*current_index >= *max_index) {
        clients = realloc(clients, 2 * sizeof(clients));
        *max_index = *max_index * 2;
    }
    memcpy(&clients[*current_index].client_addr, &new_client_addr, sizeof(struct sockaddr_in));
    clients[*current_index].id = malloc(strlen(id));
    strcpy(clients[*current_index].id, id);
    clients[*current_index].filedes = client_socket;
    clients[*current_index].no_topics = 0;
    clients[*current_index].max_topics = 40;
    clients[*current_index].missed_messages = malloc(50 * sizeof(struct msg));
    clients[*current_index].no_bufs = 0;
    clients[*current_index].sfs = calloc(30, sizeof(int));
    clients[*current_index].connected = 1;
    clients[*current_index].topics = calloc(40, sizeof(char*));
    for(int i = 0 ; i < 40; i++)
        clients[*current_index].topics[i] = calloc(51, sizeof(char));
    *current_index = *current_index + 1;
    FD_SET(client_socket, file_descriptors);
    if(client_socket > *no_filedes)
        *no_filedes = client_socket;
    printf("New client %s connected from %s:%d.\n", id, inet_ntoa(new_client_addr.sin_addr), port);
}
int main(int argc, char* argv[]) {
    struct sockaddr_in server_addr;
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if(argc != 2) {
        perror("Error at arguments, usage: ./server <PORT>");
        exit(0);
    }
    int tcpsocket = socket(AF_INET, SOCK_STREAM, 0);
    int udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(tcpsocket < 0) {
        perror("Error creating tcp socket");
        exit(0);
    }
    if(udpsocket < 0) {
        perror("Error creating udp socket");
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
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    res = bind(tcpsocket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(res < 0) {
        perror("Error binding tcp socket");
        exit(0);
    }
    res = bind(udpsocket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(res < 0) {
        perror("Error binding udp socket");
        exit(0);
    }
    listen(tcpsocket, 100);
    fd_set file_descriptors;
    FD_ZERO(&file_descriptors);
    FD_SET(STDIN_FILENO, &file_descriptors);
    FD_SET(tcpsocket, &file_descriptors);
    FD_SET(udpsocket, &file_descriptors);
    int clients_max_index = 30;
    int current_clients_index = 0;
    struct Client *clients = calloc(30, sizeof(struct Client));
    int no_filedes = max(tcpsocket, udpsocket);
    while(1) {
        fd_set old_filedescriptors;
        FD_ZERO(&old_filedescriptors);
        old_filedescriptors = file_descriptors;
        int res = select(no_filedes + 1, &old_filedescriptors, NULL, NULL, NULL);
        if(res < 0) {
            perror("Error at select");
            exit(0);
        }
        for(int i = 0; i <= no_filedes; i++) {
            if(FD_ISSET(i, &old_filedescriptors)) {
            if(i == 0) {
                handle_exit(tcpsocket, udpsocket, clients, current_clients_index);
                break;
            }
            if(i == udpsocket) {
                handle_udp_message(udpsocket, clients, current_clients_index); 
                break;
            }
            if(i == tcpsocket) {
                handle_new_client(tcpsocket, clients, &clients_max_index, &current_clients_index, &no_filedes, &file_descriptors, atoi(argv[1])); 
                break; 
            }
            else {
                handle_client_message(i, clients, current_clients_index, &no_filedes, &file_descriptors);
                break; 
            }
        }
        }
    }
}