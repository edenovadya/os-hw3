//
// Created by User on 04/06/2025.
//

#ifndef QUEUE_H
#define QUEUE_H

#endif //QUEUE_H
#include <sys/time.h>

typedef struct {
   int socket;
   struct timeval arrival;
} request;

struct Queue{
   request* next_available;
   int size;
   int capacity;
   request* arr;
   request* next_request;
};

struct Queue* make_queue(int capacity);
void enqueue(struct Queue * queue, int conffd, struct timeval time);
request* dequeue(struct Queue * queue);
void destroy_queue(struct Queue * queue);