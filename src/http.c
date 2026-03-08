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
// almost forgot to add more than txt and html file options
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

    // int send(int sockfd, const void *msg, int len, int flags);
    // sockfd - socket descriptor you want to send data to
    // msg - ptr to data you want to send
    // flags - just set flags to 0 :)

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
            send(client_socket, buffer, bytes_read, 0);
        }

        fclose(file);
}


    //============================================================================
    // Implementation of post request
    //============================================================================
    

    void handle_post(int client_socket, char *request){
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
    // Implementation of put request (now binary safe)
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
    // Implementation of delete request
    //============================================================================
    void handle_delete(int client_socket, const char *path){
        // remove same as rm filename
        if (remove(path) == 0) {

            char *response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n\r\n"
                "File deleted\n";

            send(client_socket, response, strlen(response), 0);
        } 

        else{
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
    void send_405(int client_socket){
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