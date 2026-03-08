#ifndef THREADPOOL_H
#define THREADPOOL_H

void threadpool_init(int workers);
void threadpool_add(int client_socket);

#endif