    //============================================================================
    // Include these libraries
    //============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "server.h"
#include "http.h"


    //============================================================================
    // Constants
    //============================================================================



    // declare helper function for start_server
int read_http_request(int client_socket, char *buffer, int buffer_size);


    //============================================================================
    // Start Server function
    //============================================================================

void start_server(int port) {



    // create the socket
    // int socket(int domain, int type, int protocol);
    // ipv4, stream sockets, 0 = no specific protocol
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }


    //============================================================================
    // Allow address reuse
    //============================================================================

    // This tells the OS that we are allowed to reuse the same address/port
    // even if the socket was recently used.
    // Without this, restarting the server quickly can cause: "Address already in use"
    // This happens because the OS keeps closed sockets in TIME_WAIT for a short period to ensure all packets are handled properly.
    // SO_REUSEADDR lets us bind to the port anyway.

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));



    //============================================================================
    // Bind the socket
    //============================================================================

    struct sockaddr_in server_addr;   // stores address information of the socket (IP addr, port, protocol family)
    memset(&server_addr, 0, sizeof(server_addr)); // fill server_addr with 0s before adding stuff to it
    server_addr.sin_family      = AF_INET;        // we use ipv4
    server_addr.sin_addr.s_addr = INADDR_ANY;     // accepts connections from any ip address
    server_addr.sin_port        = htons(port);    // sets port number --- htons: host to network port - converts port to correct format

    // bind assigns the address to the socket (IP and port num)
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }


    //============================================================================
    // Listen for connections
    //============================================================================

    // aka "int listen(int sockfd, int backlog);"
    // 5 is max number of connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);


    //============================================================================
    // Accept connections
    //============================================================================

    while (1) {
        // declare struct & set len
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // accept the connections 
        // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
        int client_socket = accept(
            server_socket,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
        // inet_ntoa converts ip address to ascii string



        // read the full HTTP request
        char buffer[8192];
        // int recv(int sockfd, void *buf, int len, int flags);
        int bytes = read_http_request(client_socket, buffer, sizeof(buffer));

        if (bytes <= 0) {
            perror("read_http_request failed");
            close(client_socket);
            continue;
        }

        printf("Request:\n%s\n", buffer);



        // parse GET/POST/PUT/DELETE path
        char method[16];
        char path[256];
        sscanf(buffer, "%s %s", method, path);
        printf("Method: %s\n", method);
        printf("Path: %s\n", path);

        // Prevent directory traversal attacks
        if (strstr(path, "..") != NULL) {
            send_403(client_socket);
            close(client_socket);
                continue;
}


        // map the url to the file
        char file_path[512];
        if (strcmp(path, "/") == 0) {
            strcpy(file_path, "www/index.html");
        } 
        else {
            snprintf(file_path, sizeof(file_path), "www%s", path);
        }

        // check method type here
        if (strcmp(method, "GET") == 0) {
            send_file(client_socket, file_path);
        }
        else if (strcmp(method, "POST") == 0) {
            handle_post(client_socket, buffer);
        }
        else if (strcmp(method, "PUT") == 0) {
            handle_put(client_socket, file_path, buffer);
        }
        else if (strcmp(method, "DELETE") == 0) {
            handle_delete(client_socket, file_path);
        }
        else {
            send_405(client_socket);
        }

        //---------------------------  close & print
        close(client_socket);
        printf("Client disconnected\n");
    }

}






//============================================================================
// Recv helper function
//============================================================================

    // tcp is a continous stream of bytes, may not just be 
    int read_http_request(int client_socket, char *buffer, int buffer_size) {
        int total = 0;

        while (1) {
            int bytes = recv(client_socket, buffer + total, buffer_size - total, 0);
            if (bytes <= 0) {
                return -1;
            }

            total += bytes;
            buffer[total] = '\0';

            // check if we reached end of headers
            char *header_end = strstr(buffer, "\r\n\r\n");
            if (header_end) {

                // check for Content-Length
                char *cl = strstr(buffer, "Content-Length:");
                int content_length = 0;

                if (cl) {
                    sscanf(cl, "Content-Length: %d", &content_length);
                }

                char *body = header_end + 4;
                int body_received = total - (body - buffer);

                // keep reading until full body received
                while (body_received < content_length) {
                    bytes = recv(client_socket, buffer + total, buffer_size - total, 0);
                    if (bytes <= 0) {
                        return -1;
                    }

                    total += bytes;
                    body_received += bytes;
                    buffer[total] = '\0';
                }

                break;
            }
        }

        return total;
    }