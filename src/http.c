//============================================================================
// Include these libraries
//============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "http.h"


//============================================================================
// Constants
//============================================================================

#define BUFFER_SIZE 2048



//============================================================================
// Determine content type
//============================================================================

const char *get_content_type(const char *path) {

    const char *ext = strrchr(path, '.');

    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";

    return "application/octet-stream";
}



//============================================================================
// Send File to client (binary safe)
//============================================================================

void send_file(int client_socket, const char *path) {

    FILE *file = fopen(path, "rb");

    if (!file) {

        char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found";

        send(client_socket, response, strlen(response), 0);
        return;
    }

    const char *content_type = get_content_type(path);

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char headers[256];

    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        content_type, file_size);

    send(client_socket, headers, strlen(headers), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        size_t sent = 0;
            while (sent < bytes_read)
                sent += send(client_socket, buffer + sent, bytes_read - sent, 0);
    }

    fclose(file);
}



//============================================================================
// Implementation of POST request
//============================================================================

void handle_post(int client_socket, char *request) {

    char *body = strstr(request, "\r\n\r\n");

    if (body != NULL) {
        body += 4;
        printf("POST data: %s\n", body);
    }

    char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "POST received";

    send(client_socket, response, strlen(response), 0);
}



//============================================================================
// Implementation of PUT request (binary safe)
//============================================================================

void handle_put(int client_socket, const char *path, char *request) {

    char *header_end = strstr(request, "\r\n\r\n");

    if (!header_end) {
        send_405(client_socket);
        return;
    }

    char *cl = strstr(request, "Content-Length:");
    int content_length = 0;

    if (cl) {
        sscanf(cl, "Content-Length: %d", &content_length);
    } else {
        send_405(client_socket);
        return;
    }

    char *body = header_end + 4;

    FILE *file = fopen(path, "wb");

    if (!file) {
        send_405(client_socket);
        return;
    }

    fwrite(body, 1, content_length, file);

    fclose(file);

    char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "File written\n";

    send(client_socket, response, strlen(response), 0);
}



//============================================================================
// Implementation of DELETE request
//============================================================================

void handle_delete(int client_socket, const char *path) {

    if (remove(path) == 0) {

        char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "File deleted\n";

        send(client_socket, response, strlen(response), 0);

    } else {

        char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "File not found\n";

        send(client_socket, response, strlen(response), 0);
    }
}



//============================================================================
// Implementation of 405 error
//============================================================================

void send_405(int client_socket) {

    char *response =
        "HTTP/1.1 405 Method Not Allowed\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Method Not Allowed";

    send(client_socket, response, strlen(response), 0);
}



//============================================================================
// Implementation of 403 error
//============================================================================

void send_403(int client_socket) {

    char *response =
        "HTTP/1.1 403 Forbidden\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "403 Forbidden";

    send(client_socket, response, strlen(response), 0);
}



//============================================================================
// Handle client request (called by threadpool workers)
//============================================================================

void handle_client(int client_socket) {
    printf("Thread handling request\n");   // DEBUG: shows concurrency
    char buffer[8192];

    int bytes = read_http_request(client_socket, buffer, sizeof(buffer));

    if (bytes <= 0) {
        close(client_socket);
        return;
    }

    // printf("Request:\n%s\n", buffer);

    char method[16];
    char path[256];

    sscanf(buffer, "%s %s", method, path);

    // printf("Method: %s\n", method);
    // printf("Path: %s\n", path);

    if (strstr(path, "..") != NULL) {
        send_403(client_socket);
        close(client_socket);
        return;
    }

    char file_path[512];

    if (strcmp(path, "/") == 0)
        strcpy(file_path, "www/index.html");
    else
        snprintf(file_path, sizeof(file_path), "www%s", path);

    if (strcmp(method, "GET") == 0)
        send_file(client_socket, file_path);
    else if (strcmp(method, "POST") == 0)
        handle_post(client_socket, buffer);
    else if (strcmp(method, "PUT") == 0)
        handle_put(client_socket, file_path, buffer);
    else if (strcmp(method, "DELETE") == 0)
        handle_delete(client_socket, file_path);
    else
        send_405(client_socket);

    close(client_socket);

    printf("Client disconnected\n");
}



//============================================================================
// Recv helper function
//============================================================================

int read_http_request(int client_socket, char *buffer, int buffer_size) {

    int total = 0;

    while (1) {

        int bytes = recv(client_socket, buffer + total, buffer_size - total, 0);

        if (bytes <= 0)
            return -1;

        total += bytes;

        buffer[total] = '\0';

        char *header_end = strstr(buffer, "\r\n\r\n");

        if (header_end) {

            char *cl = strstr(buffer, "Content-Length:");

            int content_length = 0;

            if (cl)
                sscanf(cl, "Content-Length: %d", &content_length);

            char *body = header_end + 4;

            int body_received = total - (body - buffer);

            while (body_received < content_length) {

                bytes = recv(client_socket,
                             buffer + total,
                             buffer_size - total,
                             0);

                if (bytes <= 0)
                    return -1;

                total += bytes;
                body_received += bytes;

                buffer[total] = '\0';
            }

            break;
        }
    }

    return total;
}