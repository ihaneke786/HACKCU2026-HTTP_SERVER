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
    // Send HTTP response to client
    //============================================================================

    // int send(int sockfd, const void *msg, int len, int flags);
    // sockfd - socket descriptor you want to send data to
    // msg - ptr to data you want to send
    // flags - just set flags to 0 :)

    void send_html(int client_socket, const char *path) {
    FILE *file = fopen(path, "r");

    // give a 404 error if the file cannot be opened
    if (!file) {
        char *response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "404 Not Found";
        // ssize_t send(int sockfd, const void *buf, size_t len, int flags);
        send(client_socket, response, strlen(response), 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    // Send HTTP headers
    char *headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n";

    send(client_socket, headers, strlen(headers), 0);

    // Send HTML content
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
    // Implementation of put request
    //============================================================================
    void handle_put(int client_socket, const char *path, char *request){
        char *body = strstr(request, "\r\n\r\n");

        if (body == NULL) {
            send_405(client_socket);
            return;
        }

        body += 4;

        FILE *file = fopen(path, "w");

        if (!file) {
            send_405(client_socket);
            return;
        }

        fprintf(file, "%s", body);
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


    