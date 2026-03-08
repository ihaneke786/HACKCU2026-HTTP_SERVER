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
#include "threadpool.h"
#include <netinet/tcp.h>


    //============================================================================
    // Start Server function
    //============================================================================

void start_server(int port) {



    // create the socket
    // int socket(int domain, int type, int protocol);
    // ipv4, stream sockets, 0 = no specific protocol
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        perror("socket");
        exit(1);
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
    if (bind(server_socket,
        (struct sockaddr *)&server_addr,
        sizeof(server_addr)) < 0) {

        perror("bind");
        exit(1);
    }


    //============================================================================
    // Listen for connections
    //============================================================================

    // aka "int listen(int sockfd, int backlog);"
    // 5 is max number of connections
    if (listen(server_socket, 16) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port %d...\n", port);

    // initialize threadpool
    threadpool_init(16);


    //============================================================================
    // Accept connections
    //============================================================================

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

    int client_socket = accept(
        server_socket,
        (struct sockaddr *)&client_addr,
        &client_len
    );

if (client_socket < 0) {
    perror("accept");
    continue;
}

int flag = 1;
setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

threadpool_add(client_socket);
    }
}