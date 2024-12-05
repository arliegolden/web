#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H
#include <stddef.h>

void serve_static_file(int client_socket, const char *path);
void handle_post_ping(int client_socket, const char *body);
void generate_directory_listing(const char *dir_path, char *output, size_t output_size);

#endif