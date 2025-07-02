#include "frogger.h"

void init_buffer(shared_buffer_t* buf) {
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;
    
    pthread_mutex_init(&buf->mutex, NULL);
    sem_init(&buf->empty, 0, BUFFER_SIZE);
    sem_init(&buf->full, 0, 0);
}

void destroy_buffer(shared_buffer_t* buf) {
    pthread_mutex_destroy(&buf->mutex);
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
}

int put_object(shared_buffer_t* buf, game_object_t obj) {
    // Prova ad acquisire un slot vuoto (non bloccante)
    if (sem_trywait(&buf->empty) != 0) {
        return 0; // Buffer pieno
    }
    
    pthread_mutex_lock(&buf->mutex);
    
    // Inserisci l'oggetto nel buffer
    buf->buffer[buf->tail] = obj;
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    buf->count++;
    
    pthread_mutex_unlock(&buf->mutex);
    
    // Segnala che c'è un nuovo elemento
    sem_post(&buf->full);
    
    return 1; // Successo
}

int get_object(shared_buffer_t* buf, game_object_t* obj) {
    // Prova ad acquisire un elemento (non bloccante)
    if (sem_trywait(&buf->full) != 0) {
        return 0; // Buffer vuoto
    }
    
    pthread_mutex_lock(&buf->mutex);
    
    // Estrai l'oggetto dal buffer
    *obj = buf->buffer[buf->head];
    buf->head = (buf->head + 1) % BUFFER_SIZE;
    buf->count--;
    
    pthread_mutex_unlock(&buf->mutex);
    
    // Segnala che c'è un slot libero
    sem_post(&buf->empty);
    
    return 1; // Successo
}

int is_valid_position(int x, int y, int width, int height) {
    if (x < 0 || y < 0) {
        return 0;
    }
    
    if (x + width > game_state.screen_width || y + height > game_state.screen_height) {
        return 0;
    }
    
    return 1;
}

int objects_collide(game_object_t* obj1, game_object_t* obj2) {
    // Controlla se due oggetti si sovrappongono
    if (obj1->x + obj1->width <= obj2->x || obj2->x + obj2->width <= obj1->x) {
        return 0; // Non si sovrappongono orizzontalmente
    }
    
    if (obj1->y + obj1->height <= obj2->y || obj2->y + obj2->height <= obj1->y) {
        return 0; // Non si sovrappongono verticalmente
    }
    
    return 1; // Collisione rilevata
}

int get_random_number(int min, int max) {
    if (min >= max) {
        return min;
    }
    
    return min + (rand() % (max - min + 1));
}

// Funzione per verificare se la rana è in acqua
int frog_in_water(game_object_t* frog) {
    if (frog->y >= game_state.river_start_y && frog->y < game_state.river_end_y) {
        // La rana è nel fiume, controlla se è su un coccodrillo
        // Questa è una versione semplificata - in una implementazione completa
        // dovremmo controllare la posizione rispetto ai coccodrilli presenti
        return 1; // Assumiamo che sia in acqua se non specificato diversamente
    }
    return 0;
}

// Funzione per verificare se la rana ha raggiunto una tana
int check_frog_in_tana(game_object_t* frog) {
    if (frog->y == game_state.tane_y) {
        for (int i = 0; i < NUM_TANE; i++) {
            int tana_x = (game_state.screen_width / (NUM_TANE + 1)) * (i + 1) - 2;
            
            // Controlla se la rana è nella posizione della tana
            if (frog->x >= tana_x && frog->x < tana_x + 4) {
                if (!game_state.tane_closed[i]) {
                    // Tana raggiunta!
                    game_state.tane_closed[i] = 1;
                    game_state.score += 100;
                    return 1;
                } else {
                    // Tana già chiusa - rana perde una vita
                    return -1;
                }
            }
        }
        // La rana è nella riga delle tane ma non in una tana valida
        return -1;
    }
    return 0;
}

// Funzione per resettare la posizione della rana
void reset_frog_position(game_object_t* frog) {
    frog->x = game_state.screen_width / 2;
    frog->y = game_state.sidewalk_y - 1;
}

// Funzione per gestire la perdita di una vita
void lose_life(void) {
    game_state.lives--;
    
    if (game_state.lives <= 0) {
        game_state.game_over = 1;
    } else {
        reset_round();
    }
}

// Funzione per calcolare il punteggio basato sul tempo rimanente
void update_score_for_time(void) {
    // Bonus per il tempo rimanente
    game_state.score += game_state.time_left * 2;
}

// Funzione per verificare le condizioni di fine round
void check_round_end_conditions(game_object_t* frog) {
    // Controlla se la rana è in una tana
    int tana_result = check_frog_in_tana(frog);
    if (tana_result == 1) {
        // Tana raggiunta con successo
        update_score_for_time();
        
        // Controlla condizione di vittoria
        if (check_win_condition()) {
            game_state.victory = 1;
            game_state.game_over = 1;
        } else {
            reset_round();
            reset_frog_position(frog);
        }
    } else if (tana_result == -1) {
        // Tana non valida o già chiusa
        lose_life();
        reset_frog_position(frog);
    }
    
    // Controlla se la rana è in acqua
    if (frog_in_water(frog)) {
        lose_life();
        reset_frog_position(frog);
    }
}

// Funzione per gestire le collisioni tra granate e proiettili
void handle_grenade_bullet_collision(game_object_t* grenade, game_object_t* bullet) {
    // Distruggi entrambi gli oggetti
    grenade->active = 0;
    bullet->active = 0;
    
    // Aggiungi punti per aver neutralizzato un proiettile
    game_state.score += 10;
}

// Funzione per gestire le collisioni tra rana e proiettili
void handle_frog_bullet_collision(game_object_t* frog, game_object_t* bullet) {
    bullet->active = 0;
    lose_life();
    reset_frog_position(frog);
}

// Funzione per verificare se un oggetto è uscito dall'area di gioco
int object_out_of_bounds(game_object_t* obj) {
    if (obj->x + obj->width < 0 || obj->x >= game_state.screen_width) {
        return 1;
    }
    
    if (obj->y + obj->height < 0 || obj->y >= game_state.screen_height) {
        return 1;
    }
    
    return 0;
}

// Funzione per pulire gli oggetti non attivi
void cleanup_inactive_objects(game_object_t* objects, int* obj_count) {
    int write_index = 0;
    
    for (int read_index = 0; read_index < *obj_count; read_index++) {
        if (objects[read_index].active && !object_out_of_bounds(&objects[read_index])) {
            if (write_index != read_index) {
                objects[write_index] = objects[read_index];
            }
            write_index++;
        }
    }
    
    *obj_count = write_index;
}

// Funzione per inizializzare un coccodrillo in una corsia specifica
void init_crocodile_in_lane(game_object_t* croc, int lane) {
    croc->type = OBJ_CROCODILE;
    croc->id = rand();
    croc->lane = lane;
    croc->width = get_random_number(CROC_MIN_WIDTH, CROC_MAX_WIDTH);
    croc->height = CROC_HEIGHT;
    croc->y = game_state.river_start_y + lane;
    croc->dir = game_state.lanes[lane].direction;
    croc->active = 1;
    
    if (croc->dir == DIR_RIGHT) {
        croc->x = -croc->width;
    } else {
        croc->x = game_state.screen_width;
    }
}

// Funzione per verificare se la rana è su un coccodrillo
int frog_on_crocodile(game_object_t* frog, game_object_t* crocodiles, int croc_count) {
    if (frog->y < game_state.river_start_y || frog->y >= game_state.river_end_y) {
        return 1; // Non nel fiume, quindi "sicura"
    }
    
    for (int i = 0; i < croc_count; i++) {
        if (crocodiles[i].active && crocodiles[i].y == frog->y) {
            if (frog->x >= crocodiles[i].x && 
                frog->x + frog->width <= crocodiles[i].x + crocodiles[i].width) {
                return 1; // Su un coccodrillo
            }
        }
    }
    
    return 0; // In acqua, non su un coccodrillo
}

// Funzione per stampare statistiche di debug (opzionale)
void print_debug_info(void) {
    static int debug_counter = 0;
    debug_counter++;
    
    if (debug_counter % 100 == 0) { // Stampa ogni 100 frame
        mvprintw(game_state.screen_height - 1, 0, "Debug: Objects active, Buffer: %d", shared_buf.count);
    }
}

// Funzione per gestire i segnali di terminazione
void signal_handler(int sig) {
    game_running = 0;
}

// Funzione per inizializzare la gestione dei segnali
void init_signal_handling(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}
