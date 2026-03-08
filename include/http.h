#ifndef HTTP_H
#define HTTP_H

// file serving
void send_file(int client_socket, const char *path);

// request handlers
void handle_post(int client_socket, char *request);
void handle_put(int client_socket, const char *path, char *request);
void handle_delete(int client_socket, const char *path);

// errors
void send_403(int client_socket);
void send_405(int client_socket);
void send_505(int client_socket);

// client handler (called by threadpool)
void handle_client(int client_socket);

// helper
int read_http_request(int client_socket, char *buffer, int buffer_size);

#endif