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
struct Client {
    struct sockaddr_in client_addr;
    int connected;
    char* id;
    int filedes;
    char** topics;
    int no_topics;
    int max_topics;
    int* sfs;
    struct msg* missed_messages;
    int no_bufs;
};
struct __attribute__ ((__packed__)) ACK {
    int valid_subscription;
    char msg[50];
};
struct __attribute__ ((__packed__)) msg {
    char ip[50];
    int port;
    char buf[1554];
};