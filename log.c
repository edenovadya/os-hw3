#include <stdlib.h>
#include <string.h>
#include "log.h"


void readers_writers_init(struct Server_Log* log) {
    log->readers_inside = 0;
    log->writers_inside = 0;
    log->writers_waiting = 0;
    cond_init(&log->read_allowed, NULL);
    cond_init(&log->write_allowed, NULL);
    mutex_init(&log->mutex_log, NULL);
}

void reader_lock(struct Server_Log* log) {
    mutex_lock(&log->mutex_log);
    while (log->writers_inside > 0 || log->writers_waiting > 0)
        cond_wait(&log->read_allowed, &log->mutex_log);
    log->readers_inside++;
    mutex_unlock(&log->mutex_log);
}
void reader_unlock(struct Server_Log* log) {
    mutex_lock(&log->mutex_log);
    log->readers_inside--;
    if (log->readers_inside == 0)
        cond_signal(&log->write_allowed);
    mutex_unlock(&log->mutex_log);
}
void writer_lock(struct Server_Log* log) {
    mutex_lock(&log->mutex_log);
    log->writers_waiting++;
    while (log->writers_inside + log->readers_inside > 0)
        cond_wait(&log->write_allowed, &log->mutex_log);
    log->writers_waiting--;
    log->writers_inside++;
    mutex_unlock(&log->mutex_log);
}
void writer_unlock(struct Server_Log* log) {
    mutex_lock(&log->mutex_log);
    log->writers_inside--;
    if (log->writers_inside == 0) {
        cond_broadcast(&log->read_allowed);
        cond_signal(&log->write_allowed);
    }
    mutex_unlock(&log->mutex_log);
}


typedef struct {
    Node* next;
    char* log_data;
} Node;

// Opaque struct definition
struct Server_Log {
    int size;
    Node* head;
    Node* tail;
    int readers;
    int writers;
    int writers_waiting;
    pthread_mutex_t mutex_log;
    pthread_cond_t read_allowed;
    pthread_cond_t write_allowed;
    // TODO: Implement internal log storage (e.g., dynamic buffer, linked list, etc.)
};

// Creates a new server log instance (stub)
server_log* create_log() {
    // TODO: Allocate and initialize internal log structure
    server_log* server = (server_log)malloc(sizeof(struct Server_Log));
    readers_writers_init(server);
    server->size = 0;
    server->head = NULL;
    server->tail = NULL;
    return server;
}

// Destroys and frees the log (stub)
void destroy_log(struct server_log* log) {
    writer_lock(log);
    // TODO: Free all internal resources used by the log
    Node* current = log->head;
    Node* delete_node;
    while(current != NULL){
        delete_node = current;
        current = current->next;
        free(delete_node->log_data);
        free(delete_node);
    }
    writer_unlock(log);
    free(log);
}

// Returns dummy log content as string (stub)
int get_log(struct server_log* log, char** dst) {
    // TODO: Return the full contents of the log as a dynamically allocated string
    // This function should handle concurrent access
    reader_lock(log);
    int len = 0;
    Node* curr = log->head;
    while (curr != NULL) {
        len += strlen(curr->log_data);
        curr = curr->next;
    }

    if(len == 0){
        const char* dummy = "Log is not implemented.\n";
        len = strlen(dummy);
        *dst = (char*)malloc(len + 1); // Allocate for caller
        if (*dst != NULL) {
            strcpy(*dst, dummy);
        }
        reader_unlock(log);
        return len;
    }
    *dst = (char*)malloc(len + 1);
    (*dst)[0] = '\0';
    curr = log->head;
    while (curr != NULL) {
        strcat(*dst, curr->log_data);
        curr = curr->next;
    }
    reader_unlock(log);
    return len;

}

// Appends a new entry to the log (no-op stub)
void add_to_log(server_log* log, const char* data, int data_len) {
    Node* new_log = (Node)malloc(sizeof(struct Node));
    char* new_log->log_data = (char)malloc(data_len);
    //add error
    for (int i = 0; i < data_len; ++i) {
        *(new_log->log_data+i) = *(data+i);
    }

    writer_lock(log);
    if(log->size == 0){
        new_log = log->head;
        new_log = log->tail;
    }else{
        log->tail->next = new_log;
        log->tail = new_log;
    }
    log->size++;
    // TODO: Append the provided data to the log
    // This function should handle concurrent access
    writer_unlock(log);
}
