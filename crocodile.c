#include "frogger.h"

void* crocodile_thread(void* arg) {
    game_object_t* croc = (game_object_t*)arg;
    int bullet_id = 2000 + croc->id; // ID univoci per i proiettili
    int shoot_timer = 0;
    int shoot_interval = get_random_number(50, 150); // Intervallo casuale di sparo
    
    while(game_running && !game_state.game_over && croc->active) {
        // Salva la posizione precedente
        croc->old_x = croc->x;
        croc->old_y = croc->y;
        
        // Muove il coccodrillo
        move_crocodile(croc);
        
        // Controlla se il coccodrillo è uscito dallo schermo
        if(croc->x < -croc->width || croc->x > game_state.screen_width) {
            croc->active = 0;
            break;
        }
        
        // Gestione dello sparo
        shoot_timer++;
        if(shoot_timer >= shoot_interval) {
            if(should_shoot(croc)) {
                create_bullet(croc, &bullet_id);
                shoot_timer = 0;
                shoot_interval = get_random_number(50, 150); // Nuovo intervallo casuale
            }
        }
        
        // Invia la posizione del coccodrillo al buffer
        if(put_object(&shared_buf, *croc) != 0) {
            break;
        }
        
        // Pausa basata sulla velocità della corsia
        int speed = game_state.lanes[croc->lane].speed;
        usleep(100000 / speed); // Più veloce = meno pausa
    }
    
    // Pulisce e termina
    free(croc);
    return NULL;
}

void move_crocodile(game_object_t* croc) {
    river_lane_t* lane = &game_state.lanes[croc->lane];
    
    if(lane->direction == DIR_LEFT) {
        croc->x -= lane->speed;
    } else {
        croc->x += lane->speed;
    }
}

int should_shoot(game_object_t* croc) {
    // Spara solo se c'è una probabilità casuale
    // e se il coccodrillo è visibile sullo schermo
    if(croc->x < 0 || croc->x >= game_state.screen_width) {
        return 0;
    }
    
    // Probabilità di sparo del 2% per ogni chiamata
    return (rand() % 100) < 2;
}

void create_bullet(game_object_t* croc, int* bullet_id) {
    pthread_t bullet_thread_handle;
    game_object_t* bullet = malloc(sizeof(game_object_t));
    
    if(!bullet) {
        return;
    }
    
    bullet->type = OBJ_BULLET;
    bullet->id = (*bullet_id)++;
    bullet->width = 1;
    bullet->height = 1;
    bullet->active = 1;
    bullet->lane = croc->lane;
    
    // Posiziona il proiettile davanti al coccodrillo
    river_lane_t* lane = &game_state.lanes[croc->lane];
    if(lane->direction == DIR_LEFT) {
        bullet->x = croc->x - 1;
        bullet->dir = DIR_LEFT;
    } else {
        bullet->x = croc->x + croc->width;
        bullet->dir = DIR_RIGHT;
    }
    bullet->y = croc->y;
    
    // Crea il thread del proiettile
    pthread_create(&bullet_thread_handle, NULL, bullet_thread, bullet);
    pthread_detach(bullet_thread_handle);
}

void* bullet_thread(void* arg) {
    game_object_t* bullet = (game_object_t*)arg;
    
    while(game_running && !game_state.game_over && bullet->active) {
        // Salva la posizione precedente
        bullet->old_x = bullet->x;
        bullet->old_y = bullet->y;
        
        // Muove il proiettile
        if(bullet->dir == DIR_LEFT) {
            bullet->x -= 2; // Velocità proiettile
        } else {
            bullet->x += 2;
        }
        
        // Controlla se è uscito dallo schermo
        if(bullet->x < 0 || bullet->x >= game_state.screen_width) {
            bullet->active = 0;
            break;
        }
        
        // Invia la posizione del proiettile al buffer
        if(put_object(&shared_buf, *bullet) != 0) {
            break;
        }
        
        usleep(80000); // 80ms di pausa
    }
    
    free(bullet);
    return NULL;
}

void* grenade_thread(void* arg) {
    game_object_t* grenade = (game_object_t*)arg;
    
    while(game_running && !game_state.game_over && grenade->active) {
        // Salva la posizione precedente
        grenade->old_x = grenade->x;
        grenade->old_y = grenade->y;
        
        // Muove la granata
        if(grenade->dir == DIR_LEFT) {
            grenade->x -= 2;
        } else {
            grenade->x += 2;
        }
        
        // Controlla se è uscita dallo schermo
        if(grenade->x < 0 || grenade->x >= game_state.screen_width) {
            grenade->active = 0;
            break;
        }
        
        // Invia la posizione della granata al buffer
        if(put_object(&shared_buf, *grenade) != 0) {
            break;
        }
        
        usleep(80000); // 80ms di pausa
    }
    
    free(grenade);
    return NULL;
}

void generate_crocodiles(void) {
    static int croc_id = 100;
    
    for(int lane = 0; lane < MIN_RIVER_LANES; lane++) {
        // Genera coccodrilli solo se non ce ne sono troppi nella corsia
        if(game_state.lanes[lane].croc_count < 3) {
            // Probabilità di generazione del 5% per ogni corsia
            if((rand() % 100) < 5) {
                create_crocodile(lane, croc_id++);
            }
        }
    }
}

void create_crocodile(int lane, int id) {
    pthread_t croc_thread_handle;
    game_object_t* croc = malloc(sizeof(game_object_t));
    
    if(!croc) {
        return;
    }
    
    croc->type = OBJ_CROCODILE;
    croc->id = id;
    croc->lane = lane;
    croc->width = get_random_number(CROC_MIN_WIDTH, CROC_MAX_WIDTH);
    croc->height = CROC_HEIGHT;
    croc->y = game_state.river_start_y + lane;
    croc->active = 1;
    
    // Posiziona il coccodrillo fuori dallo schermo
    river_lane_t* lane_info = &game_state.lanes[lane];
    if(lane_info->direction == DIR_LEFT) {
        croc->x = game_state.screen_width;
        croc->dir = DIR_LEFT;
    } else {
        croc->x = -croc->width;
        croc->dir = DIR_RIGHT;
    }
    
    game_state.lanes[lane].croc_count++;
    
    // Crea il thread del coccodrillo
    pthread_create(&croc_thread_handle, NULL, crocodile_thread, croc);
    pthread_detach(croc_thread_handle);
}
