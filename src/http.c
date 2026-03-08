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
    
    