#include "segel.h"
#include "request.h"
#include "log.h"
#include <sys/time.h>
#include <stdbool.h>
// ------------------------ Structs ------------------------

typedef struct {
    int socket;
    struct timeval arrival;
} request;

typedef struct {
    request* next_available;
    int size;
    int capacity;
    int active_requests;
    request* arr;
    request* next_request;
} Queue;

typedef struct {
    Queue* request_queue;
    server_log* log;
    threads_stats t_stats;
    int thread_id;
} worker_args;


// ------------------------ Global Sync ------------------------

pthread_mutex_t mutex;
pthread_cond_t is_empty;
pthread_cond_t is_full;
// ------------------------ Queue Functions ------------------------

Queue* make_queue(int capacity) {
    Queue* q = malloc(sizeof(Queue));
    if (!q) {
        perror("malloc failed");
        exit(1);
    }

    q->arr = malloc(sizeof(request) * capacity);
    if (!q->arr) {
        perror("malloc buffer failed");
        exit(1);
    }

    q->capacity = capacity;
    q->next_available = q->arr;
    q->next_request = q->arr;
    q->size = 0;
    q->active_requests = 0;
    return q;
}

void enqueue(Queue* q, int conffd, struct timeval time) {
    if (q->size == q->capacity) {
            //not supposed to haapen
    }
    q->next_available->socket = conffd;
    q->next_available->arrival = time;

    q->next_available++;
    if (q->next_available == q->arr + q->capacity)
        q->next_available = q->arr;
    q->size++;
}

request* dequeue(Queue* q) {
    if (q->size == 0) {
        //not supposed to happen
    }
    request* ret = q->next_request;
    q->next_request++;
    if (q->next_request == q->arr + q->capacity){
        q->next_request = q->arr;
    }
    q->size--;
    q->active_requests++;
    return ret;
}

bool isEmpty(Queue* q) {
    return q->size == 0;
}

int getSize(Queue* q) {
    return q->size;
}

void destroy_queue(Queue* q) {
    if (!q) {
        return;
    }

    if (q) {
        free(q->arr);
        free(q);
    }
}

// ------------------------ Worker Function ------------------------

void* worker(void* arg_struct) {
    worker_args* args = (worker_args*)arg_struct;

    Queue* q = args->request_queue;
    server_log* log = args->log;
    threads_stats thread_stats = args->t_stats;
    int ind = args->thread_id;

    free(args);

    thread_stats->id = ind;
    thread_stats->dynm_req = 0;
    thread_stats->stat_req = 0;
    thread_stats->total_req = 0;

    request* current_request;
    while (1) {
        pthread_mutex_lock(&mutex);
        while (isEmpty(q)) {
            pthread_cond_wait(&is_empty, &mutex);
        }

        current_request = dequeue(q);
        pthread_cond_signal(&is_full);
        struct timeval picked;
        gettimeofday(&picked, NULL);
        struct timeval dispatch_time;
        timersub(&picked, &(current_request->arrival), &dispatch_time);
        pthread_mutex_unlock(&mutex);


        requestHandle(current_request->socket, current_request->arrival, dispatch_time,
                      thread_stats, log);
        pthread_mutex_lock(&mutex);
        q->active_requests--;
        pthread_mutex_unlock(&mutex);
        Close(current_request->socket);

    }
    return NULL;
}

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


// Parses command-line arguments
void getargs(int *port, int* threads_num , int* queue_size, int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
}
// TODO: HW3 — Initialize thread pool and request queue



// This server currently handles all requests in the main thread.
// You must implement a thread pool (fixed number of worker threads)
// that process requests from a synchronized queue.

int main(int argc, char *argv[])
{

    int listenfd, connfd, port, threads_num, queue_size, clientlen;
    struct sockaddr_in clientaddr;
    getargs(&port, &threads_num, &queue_size, argc, argv);
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&is_empty, NULL);
    pthread_cond_init(&is_full, NULL);

    // Create the global server log
    server_log* log = create_log();

    //creat queue
    Queue* request_queue = make_queue(queue_size);

    //creat threads pool
    pthread_t *threads = malloc(threads_num * sizeof(pthread_t));
    for (int i = 0; i < threads_num; i++) {
        worker_args *arg_to_worker = malloc(sizeof(worker_args));
        if (arg_to_worker == NULL) {
            perror("malloc failed");
            exit(1);
        }

        arg_to_worker->thread_id = i;
        arg_to_worker->request_queue = request_queue;
        arg_to_worker->log = log;

        if (pthread_create(&threads[i], NULL, worker, arg_to_worker) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        struct timeval now;
        if (gettimeofday(&now, NULL) != 0) {
            perror("gettimeofday failed");
            exit(1);
        }


        pthread_mutex_lock(&mutex);
        while (request_queue->capacity <= request_queue->size + request_queue->active_requests) {
            pthread_cond_wait(&is_full, &mutex);
        }

        enqueue(request_queue, connfd, now);
        pthread_cond_signal(&is_empty);
        pthread_mutex_unlock(&mutex);
    }
        // TODO: HW3 — Record the request arrival time here

        /*// DEMO PURPOSE ONLY:
        // This is a dummy request handler that immediately processes
        // the request in the main thread without concurrency.
        // Replace this with logic to enqueue the connection and let
        // a worker thread process it from the queue.

        threads_stats t = malloc(sizeof(struct Threads_stats));
        t->id = 0;             // Thread ID (placeholder)
        t->stat_req = 0;       // Static request count
        t->dynm_req = 0;       // Dynamic request count
        t->total_req = 0;      // Total request count

        struct timeval arrival, dispatch;
        arrival.tv_sec = 0; arrival.tv_usec = 0;   // DEMO: dummy timestamps
        dispatch.tv_sec = 0; dispatch.tv_usec = 0; // DEMO: dummy timestamps
        // gettimeofday(&arrival, NULL);

        // Call the request handler (immediate in main thread — DEMO ONLY)
        requestHandle(connfd, arrival, dispatch, t, log);

        free(t); // Cleanup
        Close(connfd); // Close the connection
        // Clean up the server log before exiting
        destroy_log(log);*/
    for (int i=0; i<threads_num; i++) {
        pthread_join(threads[i], NULL);
    }
    destroy_queue(request_queue);
    destroy_log(log);
    free(threads);
    return 0;
}



    // TODO: HW3 — Add cleanup code for thread pool and queue

