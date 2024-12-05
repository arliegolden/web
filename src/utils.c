#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void send_response(int client_socket, const char *status, const char *content_type, 
                  const void *body, size_t body_length) {
    char header[SMALL_BUFFER];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status, content_type, body_length);

    if (send(client_socket, header, header_length, 0) == -1) {
        perror("send header failed");
        return;
    }
    
    if (send(client_socket, body, body_length, 0) == -1) {
        perror("send body failed");
        return;
    }
}

void send_simple_response(int client_socket, const char *status, const char *content_type) {
    char header[SMALL_BUFFER];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n",
        status, content_type);

    if (send(client_socket, header, header_length, 0) == -1) {
        perror("send simple response failed");
    }
}