#include "http_server.h"
#include "config.h"
#include "request_handler.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>

void *handle_client(void *arg) {
    client_info *cinfo = (client_info *)arg;
    int client_socket = cinfo->client_socket;
    char buffer[BUFFER_SIZE];
    ssize_t received;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cinfo->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    
    DEBUG("New connection from %s", client_ip);

    received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (received < 0) {
        ERROR("Failed to receive data from client %s", client_ip);
        close(client_socket);
        free(cinfo);
        pthread_exit(NULL);
    }
    buffer[received] = '\0';

    char method[SMALL_BUFFER];
    char path[SMALL_BUFFER];
    char protocol[SMALL_BUFFER];
    sscanf(buffer, "%s %s %s", method, path, protocol);

    INFO("Request from %s: %s %s %s", client_ip, method, path, protocol);
    TRACE("Full request:\n%s", buffer);
    
    if (strcasecmp(method, "GET") == 0) {
        serve_static_file(client_socket, path);
    }
    else if (strcasecmp(method, "POST") == 0 && strcmp(path, "/ping") == 0) {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4;
            handle_post_ping(client_socket, body);
        }
        else {
            send_simple_response(client_socket, "400 Bad Request", "text/plain");
        }
    }
    else {
        send_simple_response(client_socket, "501 Not Implemented", "text/plain");
    }

    close(client_socket);
    free(cinfo);
    pthread_exit(NULL);
}

int initialize_server(void) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 16) < 0) {
        perror("listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    INFO("Server is running on port %d\n", PORT);

    while (1) {
        client_info *cinfo = malloc(sizeof(client_info));
        if (!cinfo) {
            perror("malloc failed");
            continue;
        }
        
        socklen_t addr_len = sizeof(cinfo->client_addr);
        cinfo->client_socket = accept(server_fd, (struct sockaddr *)&cinfo->client_addr, &addr_len);
        
        if (cinfo->client_socket < 0) {
            perror("accept failed");
            free(cinfo);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)cinfo) != 0) {
            perror("pthread_create failed");
            close(cinfo->client_socket);
            free(cinfo);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}