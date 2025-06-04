//
// Created by User on 04/06/2025.
//
#include <Queue.h>
#include <stdio.h>

struct Queue* make_queue(int capacity) {
    struct Queue* q = malloc(sizeof(struct Queue));
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
    return q;
}

void enqueue(Queue* q, int conffd, struct timeval time) {
    if (q->size == q->capacity) {
        //not supposed to haapen
    }

    q->next_available->socket = conffd;
    q->next_available->arrival = time;

    q->next_available++;
    if (q->next_available == q->arr + q->capacity) {
        q->next_available = q->arr;
    }
    
    q->size++;
}



void destroy_queue(struct Queue* q) {
    if (!q) return;

    if (q->arr) {
        free(q->arr);
    }

    free(q);
}

request* dequeue(Queue* q) {
    if (q->size == 0) {
        //not supposed to happen
    }

    request* ret = q->next_request;

    q->next_request++;
    if (q->next_request == q->arr + q->capacity) {
        q->next_request = q->arr;
    }

    q->size--;
    return ret;
}

