#include "frogger.h"

void* frog_thread(void* arg) {
    game_object_t frog;
    int ch;
    int grenade_id = 1000; // ID univoci per le granate
    
    // Inizializza la rana
    frog.type = OBJ_FROG;
    frog.id = 0;
    frog.width = FROG_WIDTH;
    frog.height = FROG_HEIGHT;
    frog.x = game_state.screen_width / 2;
    frog.y = game_state.sidewalk_y - 1;
    frog.old_x = frog.x;
    frog.old_y = frog.y;
    frog.active = 1;
    frog.dir = DIR_UP;
    frog.lane = -1; // Non in una corsia del fiume
    
    while(game_running && !game_state.game_over) {
        // Salva la posizione precedente
        frog.old_x = frog.x;
        frog.old_y = frog.y;
        
        // Legge l'input del giocatore
        ch = getch();
        
        int new_x = frog.x;
        int new_y = frog.y;
        
        switch(ch) {
            case KEY_UP:
                new_y = frog.y - FROG_HEIGHT;
                frog.dir = DIR_UP;
                break;
            case KEY_DOWN:
                new_y = frog.y + FROG_HEIGHT;
                frog.dir = DIR_DOWN;
                break;
            case KEY_LEFT:
                new_x = frog.x - FROG_WIDTH;
                frog.dir = DIR_LEFT;
                break;
            case KEY_RIGHT:
                new_x = frog.x + FROG_WIDTH;
                frog.dir = DIR_RIGHT;
                break;
            case ' ': // Barra spaziatrice per le granate
                create_grenades(&frog, &grenade_id);
                break;
        }
        
        // Verifica se la nuova posizione è valida
        if(is_valid_position(new_x, new_y, frog.width, frog.height)) {
            frog.x = new_x;
            frog.y = new_y;
            
            // Determina in quale area si trova la rana
            update_frog_context(&frog);
        }
        
        // Invia la posizione della rana al buffer
        if(put_object(&shared_buf, frog) != 0) {
            break; // Errore nel buffer
        }
        
        usleep(50000); // 50ms di pausa
    }
    
    return NULL;
}

void create_grenades(game_object_t* frog, int* grenade_id) {
    pthread_t grenade_thread_left, grenade_thread_right;
    game_object_t* grenade_left = malloc(sizeof(game_object_t));
    game_object_t* grenade_right = malloc(sizeof(game_object_t));
    
    if(!grenade_left || !grenade_right) {
        if(grenade_left) free(grenade_left);
        if(grenade_right) free(grenade_right);
        return;
    }
    
    // Granata sinistra
    grenade_left->type = OBJ_GRENADE;
    grenade_left->id = (*grenade_id)++;
    grenade_left->x = frog->x;
    grenade_left->y = frog->y;
    grenade_left->width = 1;
    grenade_left->height = 1;
    grenade_left->dir = DIR_LEFT;
    grenade_left->active = 1;
    grenade_left->lane = frog->lane;
    
    // Granata destra
    grenade_right->type = OBJ_GRENADE;
    grenade_right->id = (*grenade_id)++;
    grenade_right->x = frog->x;
    grenade_right->y = frog->y;
    grenade_right->width = 1;
    grenade_right->height = 1;
    grenade_right->dir = DIR_RIGHT;
    grenade_right->active = 1;
    grenade_right->lane = frog->lane;
    
    // Crea i thread per le granate
    pthread_create(&grenade_thread_left, NULL, grenade_thread, grenade_left);
    pthread_create(&grenade_thread_right, NULL, grenade_thread, grenade_right);
    
    // Detach dei thread per la pulizia automatica
    pthread_detach(grenade_thread_left);
    pthread_detach(grenade_thread_right);
}

void update_frog_context(game_object_t* frog) {
    // Controlla in quale area si trova la rana
    if(frog->y <= game_state.tane_y) {
        // Area delle tane
        check_tana_collision(frog);
    } else if(frog->y == game_state.grass_y) {
        // Sponda d'erba - sicura
        frog->lane = -1;
    } else if(frog->y >= game_state.river_start_y && frog->y <= game_state.river_end_y) {
        // Nel fiume - determina la corsia
        frog->lane = frog->y - game_state.river_start_y;
        
        // Controlla se è caduta in acqua (non su un coccodrillo)
        if(!is_on_crocodile(frog)) {
            // Rana caduta in acqua
            lose_life();
            return;
        }
    } else if(frog->y >= game_state.sidewalk_y) {
        // Marciapiede - sicuro
        frog->lane = -1;
    }
}

void check_tana_collision(game_object_t* frog) {
    int tana_width = game_state.screen_width / NUM_TANE;
    
    for(int i = 0; i < NUM_TANE; i++) {
        int tana_x = i * tana_width;
        int tana_x_end = tana_x + tana_width;
        
        if(frog->x >= tana_x && frog->x < tana_x_end) {
            if(!game_state.tane_closed[i]) {
                // Tana aperta - vittoria della manche
                game_state.tane_closed[i] = 1;
                game_state.score += 100 + game_state.time_left * 2;
                
                if(check_win_condition()) {
                    game_state.victory = 1;
                    game_state.game_over = 1;
                } else {
                    reset_round();
                }
                return;
            } else {
                // Tana chiusa - perde vita
                lose_life();
                return;
            }
        }
    }
    
    // Non è in una tana valida - perde vita
    lose_life();
}

int is_on_crocodile(game_object_t* frog) {
    // Questa funzione sarà implementata nel modulo delle collisioni
    // Per ora ritorna sempre true per evitare crash
    return 1;
}

void lose_life(void) {
    game_state.lives--;
    if(game_state.lives <= 0) {
        game_state.game_over = 1;
    } else {
        reset_round();
    }
}
