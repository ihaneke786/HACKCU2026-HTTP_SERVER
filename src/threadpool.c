#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "threadpool.h"
#include "http.h"

#define QUEUE_SIZE 64

typedef struct {

    int sockets[QUEUE_SIZE];
    int front;
    int rear;

    pthread_mutex_t lock;
    pthread_cond_t cond;

} job_queue;

static job_queue queue;

static void queue_init() {

    queue.front = 0;
    queue.rear = 0;

    pthread_mutex_init(&queue.lock, NULL);
    pthread_cond_init(&queue.cond, NULL);
}

static void enqueue(int socket)
{
    pthread_mutex_lock(&queue.lock);

    int next = (queue.rear + 1) % QUEUE_SIZE;

    while (next == queue.front)
        pthread_cond_wait(&queue.cond, &queue.lock);

    queue.sockets[queue.rear] = socket;
    queue.rear = next;

    pthread_cond_signal(&queue.cond);

    pthread_mutex_unlock(&queue.lock);
}

static int dequeue() {

    pthread_mutex_lock(&queue.lock);

    while (queue.front == queue.rear)
        pthread_cond_wait(&queue.cond, &queue.lock);

    int socket = queue.sockets[queue.front];
    queue.front = (queue.front + 1) % QUEUE_SIZE;

    pthread_mutex_unlock(&queue.lock);

    return socket;
}

static void *worker(void *arg) {

    (void)arg;

    while (1) {

        int client_socket = dequeue();

        handle_client(client_socket);
    }

    return NULL;
}

void threadpool_init(int workers) {

    queue_init();

    pthread_t thread;

    for (int i = 0; i < workers; i++) {

        if (pthread_create(&thread, NULL, worker, NULL) != 0) {
            perror("pthread_create");
            exit(1);
        }

        pthread_detach(thread);
    }
}

void threadpool_add(int client_socket) {

    enqueue(client_socket);
}