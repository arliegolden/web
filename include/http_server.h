#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <netinet/in.h>

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_info;

int initialize_server(void);
void *handle_client(void *arg);

#endif