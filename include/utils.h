#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>

void send_response(int client_socket, const char *status, const char *content_type, 
                  const void *body, size_t body_length);
void send_simple_response(int client_socket, const char *status, const char *content_type);

#endif