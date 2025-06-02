#include <stdlib.h>
#include <string.h>
#include "log.h"

int readers;
int writers;
int writers_waiting;
pthread_mutex_t mutex_log;
pthread_cond_t read_allowed;
pthread_cond_t write_allowed;

void readers_writers_init() {
    readers_inside = 0;
    writers_inside = 0;
    writers_waiting = 0;
    cond_init(&read_allowed, NULL);
    cond_init(&write_allowed, NULL);
    mutex_init(&mutex_log, NULL);
}

void reader_lock() {
    mutex_lock(&mutex_log);
    while (writers_inside > 0 || writers_waiting > 0)
        cond_wait(&read_allowed, &global_lock);
    readers_inside++;
    mutex_unlock(&mutex_log);
}
void reader_unlock() {
    mutex_lock(&mutex_log);
    readers_inside--;
    if (readers_inside == 0)
        cond_signal(&write_allowed);
    mutex_unlock(&mutex_log);
}
void writer_lock() {
    mutex_lock(&mutex_log);
    writers_waiting++;
    while (writers_inside + readers_inside > 0)
        cond_wait(&write_allowed, &mutex_log);
    writers_waiting--;
    writers_inside++;
    mutex_unlock(&mutex_log);
}
void writer_unlock() {
    mutex_lock(&mutex_log);
    writers_inside--;
    if (writers_inside == 0) {
        cond_broadcast(&read_allowed);
        cond_signal(&write_allowed);
    }
    mutex_unlock(&mutex_log);
}


typedef struct {
    Node* next;
    string log;
} Node;

// Opaque struct definition
struct Server_Log {
    int size;
    Node* head;
    Node* tail;
    // TODO: Implement internal log storage (e.g., dynamic buffer, linked list, etc.)
};

// Creates a new server log instance (stub)
server_log create_log() {
    // TODO: Allocate and initialize internal log structure
    readers_writers_init();
    server_log server = (server_log)malloc(sizeof(struct Server_Log));
    server.size = 0;
    server.head = NULL;
    server.tail = NULL;
    return *server;
}

// Destroys and frees the log (stub)
void destroy_log(server_log log) {
    writer_lock();
    // TODO: Free all internal resources used by the log
    Node* current = log.head;
    Node* delete_node;
    while(current != NULL){
        delete_node = current;
        current = current->next;
        free(delete_node);
    }
    writer_unlock;
}

// Returns dummy log content as string (stub)
int get_log(server_log log, char** dst) {
    // TODO: Return the full contents of the log as a dynamically allocated string
    // This function should handle concurrent access

    const char* dummy = "Log is not implemented.\n";
    int len = strlen(dummy);
    *dst = (char*)malloc(len + 1); // Allocate for caller
    if (*dst != NULL) {
        strcpy(*dst, dummy);
    }
    return len;
}

// Appends a new entry to the log (no-op stub)
void add_to_log(server_log log, const char* data, int data_len) {
    if(log.size == 0){
        
    }
    // TODO: Append the provided data to the log
    // This function should handle concurrent access
}
