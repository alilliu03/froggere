#include "frogger.h"

void init_buffer(shared_buffer_t* buf) {
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;
    
    pthread_mutex_init(&buf->mutex, NULL);
    sem_init(&buf->empty, 0, BUFFER_SIZE);
    sem_init(&buf->full, 0, 0);
    
    // Inizializza tutti gli oggetti del buffer
    for(int i = 0; i < BUFFER_SIZE; i++) {
        buf->buffer[i].active = 0;
        buf->buffer[i].type = OBJ_FROG;
        buf->buffer[i].x = 0;
        buf->buffer[i].y = 0;
        buf->buffer[i].id = -1;
    }
}

void destroy_buffer(shared_buffer_t* buf) {
    pthread_mutex_destroy(&buf->mutex);
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
}

int put_object(shared_buffer_t* buf, game_object_t obj) {
    // Attende che ci sia spazio nel buffer
    if(sem_wait(&buf->empty) != 0) {
        return -1;
    }
    
    // Acquisisce il mutex per accesso esclusivo
    pthread_mutex_lock(&buf->mutex);
    
    // Inserisce l'oggetto nel buffer
    buf->buffer[buf->tail] = obj;
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    buf->count++;
    
    pthread_mutex_unlock(&buf->mutex);
    
    // Segnala che c'è un nuovo elemento
    sem_post(&buf->full);
    
    return 0;
}

int get_object(shared_buffer_t* buf, game_object_t* obj) {
    // Attende che ci sia almeno un elemento
    if(sem_wait(&buf->full) != 0) {
        return -1;
    }
    
    // Acquisisce il mutex per accesso esclusivo
    pthread_mutex_lock(&buf->mutex);
    
    // Estrae l'oggetto dal buffer
    *obj = buf->buffer[buf->head];
    buf->head = (buf->head + 1) % BUFFER_SIZE;
    buf->count--;
    
    pthread_mutex_unlock(&buf->mutex);
    
    // Segnala che c'è spazio disponibile
    sem_post(&buf->empty);
    
    return 0;
}

// Funzione non bloccante per controllare se il buffer è vuoto
int is_buffer_empty(shared_buffer_t* buf) {
    int empty;
    pthread_mutex_lock(&buf->mutex);
    empty = (buf->count == 0);
    pthread_mutex_unlock(&buf->mutex);
    return empty;
}

// Funzione non bloccante per controllare se il buffer è pieno
int is_buffer_full(shared_buffer_t* buf) {
    int full;
    pthread_mutex_lock(&buf->mutex);
    full = (buf->count == BUFFER_SIZE);
    pthread_mutex_unlock(&buf->mutex);
    return full;
}
