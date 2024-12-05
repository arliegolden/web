#include "request_handler.h"
#include "config.h"
#include "mime_types.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

void handle_post_ping(int client_socket, const char *body) {
    (void)body;
    char response_body[SMALL_BUFFER];
    int response_length = snprintf(response_body, sizeof(response_body),
        "{ \"response\": \"Pong!\" }");
    send_response(client_socket, "200 OK", "application/json", response_body, response_length);
}

void generate_directory_listing(const char *dir_path, char *output, size_t output_size) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[SMALL_BUFFER];
    char time_str[80];
    
    int written = snprintf(output, output_size,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Directory listing</title>\n"
        "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jquery/3.7.1/jquery.min.js\"></script>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Directory listing for %s</h1>\n"
        "<table>\n"
        "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n",
    dir_path);

    output += written;
    output_size -= written;

    dir = opendir(dir_path);
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0) continue;
            
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            
            if (stat(full_path, &file_stat) == 0) {
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", 
                        localtime(&file_stat.st_mtime));
                
                if (S_ISDIR(file_stat.st_mode)) {
                    written = snprintf(output, output_size,
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>-</td><td>%s</td></tr>\n",
                        entry->d_name, entry->d_name, time_str);
                } else {
                    written = snprintf(output, output_size,
                        "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td><td>%s</td></tr>\n",
                        entry->d_name, entry->d_name, file_stat.st_size, time_str);
                }
                output += written;
                output_size -= written;
            }
        }
        closedir(dir);
    }

    snprintf(output, output_size,
        "</table>\n"
        "</body>\n"
        "</html>");
}

void serve_static_file(int client_socket, const char *path) {
    if (strstr(path, "..")) {
        send_simple_response(client_socket, "400 Bad Request", "text/plain");
        return;
    }

    char file_path[SMALL_BUFFER] = ROOT_DIR;
    strcat(file_path, path);

    struct stat path_stat;
    if (stat(file_path, &path_stat) == -1) {
        send_simple_response(client_socket, "404 Not Found", "text/plain");
        return;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        char *listing = malloc(BUFFER_SIZE * 4);
        if (!listing) {
            send_simple_response(client_socket, "500 Internal Server Error", "text/plain");
            return;
        }

        generate_directory_listing(file_path, listing, BUFFER_SIZE * 4);
        send_response(client_socket, "200 OK", "text/html", listing, strlen(listing));
        free(listing);
        return;
    }

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        send_simple_response(client_socket, "404 Not Found", "text/plain");
        return;
    }

    const char *mime_type = get_mime_type(file_path);
    
    struct stat st;
    fstat(file_fd, &st);
    long file_size = st.st_size;

    char header[SMALL_BUFFER];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        mime_type, file_size);

    send(client_socket, header, header_length, 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(file_fd, buffer, sizeof(buffer))) > 0) {
        send(client_socket, buffer, bytes, 0);
    }

    close(file_fd);
}