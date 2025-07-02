#include "frogger.h"

// Array per tenere traccia degli oggetti attivi
static game_object_t active_objects[500];
static int object_count = 0;
static pthread_mutex_t objects_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_object_list(game_object_t obj) {
    pthread_mutex_lock(&objects_mutex);
    
    // Cerca se l'oggetto esiste già
    int found = -1;
    for(int i = 0; i < object_count; i++) {
        if(active_objects[i].id == obj.id && active_objects[i].type == obj.type) {
            found = i;
            break;
        }
    }
    
    if(obj.active) {
        if(found >= 0) {
            // Aggiorna oggetto esistente
            active_objects[found] = obj;
        } else {
            // Aggiunge nuovo oggetto
            if(object_count < 500) {
                active_objects[object_count] = obj;
                object_count++;
            }
        }
    } else {
        // Rimuove oggetto inattivo
        if(found >= 0) {
            for(int i = found; i < object_count - 1; i++) {
                active_objects[i] = active_objects[i + 1];
            }
            object_count--;
        }
    }
    
    pthread_mutex_unlock(&objects_mutex);
}

void check_all_collisions(void) {
    pthread_mutex_lock(&objects_mutex);
    
    for(int i = 0; i < object_count; i++) {
        for(int j = i + 1; j < object_count; j++) {
            if(objects_collide(&active_objects[i], &active_objects[j])) {
                handle_collision(&active_objects[i], &active_objects[j]);
            }
        }
    }
    
    // Controlla collisioni specifiche della rana
    game_object_t* frog = find_object_by_type(OBJ_FROG);
    if(frog) {
        check_frog_specific_collisions(frog);
    }
    
    pthread_mutex_unlock(&objects_mutex);
}

int objects_collide(game_object_t* obj1, game_object_t* obj2) {
    if(!obj1->active || !obj2->active) {
        return 0;
    }
    
    // Controllo collisione AABB (Axis-Aligned Bounding Box)
    int obj1_right = obj1->x + obj1->width - 1;
    int obj1_bottom = obj1->y + obj1->height - 1;
    int obj2_right = obj2->x + obj2->width - 1;
    int obj2_bottom = obj2->y + obj2->height - 1;
    
    return !(obj1->x > obj2_right || 
             obj2->x > obj1_right || 
             obj1->y > obj2_bottom || 
             obj2->y > obj1_bottom);
}

void handle_collision(game_object_t* obj1, game_object_t* obj2) {
    // Determina i tipi di oggetti coinvolti
    game_object_t* frog = NULL;
    game_object_t* croc = NULL;
    game_object_t* bullet = NULL;
    game_object_t* grenade = NULL;
    
    if(obj1->type == OBJ_FROG) frog = obj1;
    else if(obj2->type == OBJ_FROG) frog = obj2;
    
    if(obj1->type == OBJ_CROCODILE) croc = obj1;
    else if(obj2->type == OBJ_CROCODILE) croc = obj2;
    
    if(obj1->type == OBJ_BULLET) bullet = obj1;
    else if(obj2->type == OBJ_BULLET) bullet = obj2;
    
    if(obj1->type == OBJ_GRENADE) grenade = obj1;
    else if(obj2->type == OBJ_GRENADE) grenade = obj2;
    
    // Gestisce i diversi tipi di collisione
    if(frog && bullet) {
        // Rana colpita da proiettile
        handle_frog_bullet_collision(frog, bullet);
    }
    else if(grenade && bullet) {
        // Granata neutralizza proiettile
        handle_grenade_bullet_collision(grenade, bullet);
    }
    else if(grenade && croc) {
        // Granata attraversa coccodrillo
        handle_grenade_crocodile_collision(grenade, croc);
    }
}

void handle_frog_bullet_collision(game_object_t* frog, game_object_t* bullet) {
    // La rana perde una vita
    bullet->active = 0;
    
    // Segnala la perdita di vita
    game_object_t collision_event;
    collision_event.type = OBJ_COLLISION;
    collision_event.id = -1;
    collision_event.x = frog->x;
    collision_event.y = frog->y;
    collision_event.active = 1;
    
    put_object(&shared_buf, collision_event);
    
    lose_life();
}

void handle_grenade_bullet_collision(game_object_t* grenade, game_object_t* bullet) {
    // Entrambi gli oggetti vengono distrutti
    grenade->active = 0;
    bullet->active = 0;
    
    // Aggiorna il punteggio
    game_state.score += 10;
    
    // Crea effetto esplosione
    create_explosion_effect(grenade->x, grenade->y);
}

void handle_grenade_crocodile_collision(game_object_t* grenade, game_object_t* croc) {
    // La granata passa attraverso il coccodrillo
    // Aggiorna il punteggio per aver colpito un coccodrillo
    game_state.score += 5;
    
    // Crea effetto visivo
    create_hit_effect(croc->x + croc->width/2, croc->y);
}

void check_frog_specific_collisions(game_object_t* frog) {
    if(!frog || !frog->active) return;
    
    // Controlla se la rana è nel fiume
    if(frog->y >= game_state.river_start_y && frog->y <= game_state.river_end_y) {
        if(!is_frog_on_crocodile(frog)) {
            // Rana in acqua - perde vita
            lose_life();
        }
    }
}

int is_frog_on_crocodile(game_object_t* frog) {
    for(int i = 0; i < object_count; i++) {
        if(active_objects[i].type == OBJ_CROCODILE && 
           active_objects[i].active &&
           active_objects[i].y == frog->y) {
            
            // Controlla se la rana è sopra questo coccodrillo
            if(frog->x >= active_objects[i].x && 
               frog->x < active_objects[i].x + active_objects[i].width) {
                return 1;
            }
        }
    }
    return 0;
}

game_object_t* find_object_by_type(obj_type_t type) {
    for(int i = 0; i < object_count; i++) {
        if(active_objects[i].type == type && active_objects[i].active) {
            return &active_objects[i];
        }
    }
    return NULL;
}

game_object_t* find_object_by_id(int id, obj_type_t type) {
    for(int i = 0; i < object_count; i++) {
        if(active_objects[i].id == id && 
           active_objects[i].type == type && 
           active_objects[i].active) {
            return &active_objects[i];
        }
    }
    return NULL;
}

void create_explosion_effect(int x, int y) {
    // Crea un effetto visivo temporaneo
    pthread_mutex_lock(&screen_mutex);
    mvprintw(y, x, "*BOOM*");
    refresh();
    pthread_mutex_unlock(&screen_mutex);
    
    // L'effetto sarà cancellato dal prossimo refresh
}

void create_hit_effect(int x, int y) {
    // Crea un effetto visivo temporaneo
    pthread_mutex_lock(&screen_mutex);
    mvaddch(y, x, '+');
    refresh();
    pthread_mutex_unlock(&screen_mutex);
}

void cleanup_inactive_objects(void) {
    pthread_mutex_lock(&objects_mutex);
    
    // Rimuove tutti gli oggetti inattivi
    int new_count = 0;
    for(int i = 0; i < object_count; i++) {
        if(active_objects[i].active) {
            if(new_count != i) {
                active_objects[new_count] = active_objects[i];
            }
            new_count++;
        }
    }
    object_count = new_count;
    
    pthread_mutex_unlock(&objects_mutex);
}
