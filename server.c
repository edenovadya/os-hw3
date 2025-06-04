#include "segel.h"
#include "request.h"
#include "log.h"
#include <sys/time.h>

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


typedef struct {
    int socket;
    struct timeval arrival;
} request;

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

typedef struct {
    struct queue* request_queue;
    server_log* log;
    pthread_mutex_t* mutex;
    pthread_cond_t* cond;
    threads_stats t_stats;
    int thread_id;
} worker_args;

extern pthread_mutex_t mutex;
extern pthread_cond_t is_empty;
extern pthread_cond_t is_full;


// This server currently handles all requests in the main thread.
// You must implement a thread pool (fixed number of worker threads)
// that process requests from a synchronized queue.

int main(int argc, char *argv[])
{

    int listenfd, connfd, port, threads_num, queue_size, clientlen;
    struct sockaddr_in clientaddr;
    struct timeval arrival_time;
    getargs(&port, &threads_num, &queue_size, argc, argv);
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&is_empty, NULL);
    pthread_cond_init(&is_full, NULL);

    // Create the global server log
    server_log* log = create_log();

    //creat queue
    Queue* request_queue = make_queue();

    //creat threads pool
    pthread_t *threads = malloc(threads_num * sizeof(pthread_t));
    for (int i = 0; i < threads_num; i++) {
        worker_args *arg_to_worker = malloc(sizeof(worker_args));
        if (arg_to_worker == NULL) {
            perror("malloc failed");
            exit(1);
        }

        arg_to_worker->cond = &is_empty;
        arg_to_worker->thread_id = i;
        arg_to_worker->request_queue = request_queue;
        arg_to_worker->log = log;
        arg_to_worker->mutex = &mutex;

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
        }


        pthread_mutex_lock(&mutex);
        while (getSize(request_queue) >= queue_size) {
            pthread_cond_wait(&is_full, &mutex);
        }

        enqueue(request_queue, connfd, now);
        pthread_cond_signal(&is_empty);
        pthread_mutex_unlock(&mutex);
    }
        // TODO: HW3 — Record the request arrival time here

        // DEMO PURPOSE ONLY:
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
    }

    // Clean up the server log before exiting
    destroy_log(log);

    // TODO: HW3 — Add cleanup code for thread pool and queue
}
struct queue;
void* worker(void* arg_struct) {
    worker_args* args = (worker_args*)arg_struct;

    struct queue* q = args->q;
    server_log* log = args->log;
    pthread_mutex_t* mutex = args->mutex;
    pthread_cond_t* is_empty = args->is_empty;
    pthread_cond_t* is_full = args->is_full;
    threads_stats thread_stats = args->t_stats;
    int ind = args->thread_index;

    thread_stats->id = ind;
    thread_stats->dynm_req = 0;
    thread_stats->stat_req = 0;
    thread_stats->total_req = 0;

    request current_request;
    while (1) {
        pthread_mutex_lock(mutex);
        while (isEmpty(q)) {
            pthread_cond_wait(is_empty, mutex);
        }

        current_request = dequeue(q);
        pthread_cond_signal(is_full);
        pthread_mutex_unlock(mutex);

        struct timeval now;
        gettimeofday(&now, NULL);

        requestHandle(current_request.socket, current_request.arrival, now,
                      thread_stats, *log);
    }
    return NULL;
}
