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
// Send helper (handles partial TCP sends)
//============================================================================

void send_all(int socket,const void *buf,size_t len)
{
    size_t total = 0;

    while (total < len)
    {
        ssize_t sent = send(socket,
                            (char*)buf + total,
                            len - total,
                            0);

        if (sent <= 0)
            return;

        total += sent;
    }
}


//============================================================================
// Send file (binary safe)
//============================================================================

void send_file(int client_socket,const char *path)
{
    FILE *file = fopen(path,"rb");

    if (!file){
        char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "404 Not Found";

        send_all(client_socket,response,strlen(response));
        return;
    }

    const char *type = get_content_type(path);

    fseek(file,0,SEEK_END);
    long size = ftell(file);
    rewind(file);

    char headers[256];

    snprintf(headers,sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: keep-alive\r\n"
        "\r\n",
        type,size);

    send_all(client_socket,headers,strlen(headers));

    char buffer[BUFFER_SIZE];
    size_t bytes;

    while ((bytes = fread(buffer,1,BUFFER_SIZE,file)) > 0)
        send_all(client_socket,buffer,bytes);

    fclose(file);
}



//============================================================================
// POST
//============================================================================

void handle_post(int client_socket,char *request)
{
    char *body = strstr(request,"\r\n\r\n");

    if (body)
    {
        body += 4;
        printf("POST data: %s\n",body);
    }

    char *response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "POST received";

    send_all(client_socket,response,strlen(response));
}



//============================================================================
// Implementation of PUT request (binary safe)
//============================================================================

void handle_put(int client_socket,const char *path,char *request)
{
    char *header_end = strstr(request,"\r\n\r\n");

    if (!header_end)
    {
        send_405(client_socket);
        return;
    }

    char *cl = strstr(request,"Content-Length:");

    int length = 0;

    if (cl)
        sscanf(cl,"Content-Length: %d",&length);

    char *body = header_end + 4;

    FILE *file = fopen(path,"wb");

    if (!file)
    {
        send_405(client_socket);
        return;
    }

    fwrite(body,1,length,file);

    fclose(file);

    char *response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 12\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "File written";

    send_all(client_socket,response,strlen(response));
}



//============================================================================
// Implementation of DELETE request
//============================================================================

void handle_delete(int client_socket,const char *path)
{
    if (remove(path) == 0)
    {
        char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "File deleted";

        send_all(client_socket,response,strlen(response));
    }
    else
    {
        char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 14\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "File not found";

        send_all(client_socket,response,strlen(response));
    }
}


//============================================================================
// Implementation of 505 error
//============================================================================

void send_505(int socket)
{
    char *response =
    "HTTP/1.1 505 HTTP Version Not Supported\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 30\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "505 HTTP Version Not Supported";

    send_all(socket,response,strlen(response));
}



//============================================================================
// Implementation of 405 error
//============================================================================

void send_405(int socket)
{
    char *response =
    "HTTP/1.1 405 Method Not Allowed\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 18\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "Method Not Allowed";

    send_all(socket,response,strlen(response));
}



//============================================================================
// Implementation of 403 error
//============================================================================

void send_403(int socket)
{
    char *response =
    "HTTP/1.1 403 Forbidden\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "403 Forbidden";

    send_all(socket,response,strlen(response));
}




//============================================================================
// Handle client (persistent connection)
//============================================================================

void handle_client(int client_socket)
{
    char buffer[8192];

    while (1)
    {
        int bytes = read_http_request(client_socket,buffer,sizeof(buffer));

        if (bytes <= 0)
            break;

        char method[16];
        char path[256];
        char version[16];

        sscanf(buffer,"%s %s %s",method,path,version);

        if (strcmp(version,"HTTP/1.1") &&
            strcmp(version,"HTTP/1.0"))
        {
            send_505(client_socket);
            break;
        }

        if (strstr(path,".."))
        {
            send_403(client_socket);
            break;
        }

        char file_path[512];

        if (!strcmp(path,"/"))
            strcpy(file_path,"www/index.html");
        else
            snprintf(file_path,sizeof(file_path),"www%s",path);

        if (!strcmp(method,"GET"))
            send_file(client_socket,file_path);

        else if (!strcmp(method,"POST"))
            handle_post(client_socket,buffer);

        else if (!strcmp(method,"PUT"))
            handle_put(client_socket,file_path,buffer);

        else if (!strcmp(method,"DELETE"))
            handle_delete(client_socket,file_path);

        else
            send_405(client_socket);

        if (strstr(buffer,"Connection: close"))
            break;
    }

    close(client_socket);
}




//============================================================================
// Recv helper function(Read HTTP request)
//============================================================================

int read_http_request(int client_socket,char *buffer,int size)
{
    int total = 0;

    while (1)
    {
        int bytes = recv(client_socket,
                 buffer + total,
                 size - total - 1,
                 0);

                if (bytes <= 0)
                    return -1;

                total += bytes;

                if (total >= size - 1)
                    return -1;

                buffer[total] = '\0';

        char *header_end = strstr(buffer,"\r\n\r\n");

        if (header_end)
        {
            char *cl = strstr(buffer,"Content-Length:");

            int length = 0;

            if (cl)
                sscanf(cl,"Content-Length: %d",&length);

            char *body = header_end + 4;

            int received = total - (body - buffer);

            while (received < length)
            {
                bytes = recv(client_socket,
                    buffer + total,
                    size - total - 1,
                    0);

                if (bytes <= 0)
                    return -1;

                total += bytes;
                received += bytes;

                buffer[total] = '\0';
            }

            break;
        }
    }

    return total;
}