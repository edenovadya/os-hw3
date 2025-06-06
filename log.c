#include <stdlib.h>
#include <string.h>
#include "log.h"
#include <pthread.h>
#include <stdio.h>


// Opaque struct definition

    // TODO: Implement internal log storage (e.g., dynamic buffer, linked list, etc.)

void readers_writers_init(struct Server_Log* log) {
    log->readers_inside = 0;
    log->writers_inside = 0;
    log->writers_waiting = 0;
    pthread_cond_init(&log->read_allowed, NULL);
    pthread_cond_init(&log->write_allowed, NULL);
    pthread_mutex_init(&log->mutex_log, NULL);
}

void reader_lock(struct Server_Log* log) {
    pthread_mutex_lock(&log->mutex_log);
    while (log->writers_inside > 0 || log->writers_waiting > 0)
        pthread_cond_wait(&log->read_allowed, &log->mutex_log);
    log->readers_inside++;
    pthread_mutex_unlock(&log->mutex_log);
}

void reader_unlock(struct Server_Log* log) {
    pthread_mutex_lock(&log->mutex_log);
    log->readers_inside--;
    if (log->readers_inside == 0)
        pthread_cond_signal(&log->write_allowed);
    pthread_mutex_unlock(&log->mutex_log);
}
void writer_lock(struct Server_Log* log) {
    pthread_mutex_lock(&log->mutex_log);
    log->writers_waiting++;
    while (log->writers_inside + log->readers_inside > 0)
        pthread_cond_wait(&log->write_allowed, &log->mutex_log);
    log->writers_waiting--;
    log->writers_inside++;
    pthread_mutex_unlock(&log->mutex_log);
}
void writer_unlock(struct Server_Log* log) {
    pthread_mutex_lock(&log->mutex_log);
    log->writers_inside--;
    if (log->writers_inside == 0) { ///todo where is the writers preferance?
        pthread_cond_broadcast(&log->read_allowed);
        pthread_cond_signal(&log->write_allowed);
    }
    pthread_mutex_unlock(&log->mutex_log);
}


// Creates a new server log instance (stub)
server_log* create_log() {
    // TODO: Allocate and initialize internal log structure
    struct Server_Log* server = malloc(sizeof(struct Server_Log));
    if (!server) {
        perror("malloc failed");
        exit(1);
    }
    readers_writers_init(server);
    server->size = 0;
    server->head = NULL;
    server->tail = NULL;
    return server;
}

// Destroys and frees the log (stub)
    // TODO: Free all internal resources used by the log
void destroy_log(server_log* log) {
    if (log == NULL) {
        perror("destroy failed");
        exit(1);
    }

    writer_lock(log);

    Node* current = log->head;
    Node* delete_node;

    while(current != NULL) {
        delete_node = current;
        current = current->next;
        free(delete_node->log_data);
        free(delete_node);
    }

    writer_unlock(log);

    pthread_mutex_destroy(&log->mutex_log);
    pthread_cond_destroy(&log->read_allowed);
    pthread_cond_destroy(&log->write_allowed);

    free(log);
}


// Returns dummy log content as string (stub)
// TODO: Return the full contents of the log as a dynamically allocated string
int get_log(server_log* log, char** dst) {
    reader_lock(log);

    int len = 0;
    Node* curr = log->head;

    while (curr != NULL) {
        len += strlen(curr->log_data);
        curr = curr->next;
    }

    if (len == 0) {
        const char* dummy = "Log is not implemented.\n";
        len = strlen(dummy);
        *dst = (char*)malloc(len + 1);
        if (*dst == NULL) {
            perror("malloc failed");
            exit(1);
        }
        strcpy(*dst, dummy);
        reader_unlock(log);
        return len;
    }

    *dst = (char*)malloc(len + 1);
    if (*dst == NULL) {
        reader_unlock(log);
        perror("malloc failed");
        exit(1);
    }

    char* ptr = *dst;
    curr = log->head;
    while (curr != NULL) {
        size_t l = strlen(curr->log_data);
        memcpy(ptr, curr->log_data, l);
        ptr += l;
        curr = curr->next;
    }
    *ptr = '\0';

    reader_unlock(log);
    return len;
}

// Appends a new entry to the log (no-op stub)
void add_to_log(server_log* log, const char* data, int data_len) {
    Node* new_node = malloc(sizeof(Node));
    new_node->log_data = malloc(data_len + 1);
    memcpy(new_node->log_data, data, data_len);
    new_node->log_data[data_len] = '\0';
    new_node->next = NULL;

    writer_lock(log);

    if (log->size == 0) {
        log->head = new_node;
        log->tail = new_node;
    } else {
        log->tail->next = new_node;
        log->tail = new_node;
    }

    log->size++;

    writer_unlock(log);
}
