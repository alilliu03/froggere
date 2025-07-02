#include "frogger.h"

void* game_manager_thread(void* arg) {
    game_object_t obj;
    int generation_counter = 0;
    
    while(game_running && !game_state.game_over) {
        // Controlla se ci sono oggetti nel buffer
        while(!is_buffer_empty(&shared_buf)) {
            if(get_object(&shared_buf, &obj) == 0) {
                // Aggiorna la lista degli oggetti attivi
                update_object_list(obj);
                
                // Disegna l'oggetto
                draw_object(&obj);
            }
        }
        
        // Controlla le collisioni
        check_all_collisions();
        
        // Genera nuovi coccodrilli periodicamente
        generation_counter++;
        if(generation_counter >= 20) { // Ogni ~1 secondo
            generate_crocodiles();
            generation_counter = 0;
        }
        
        // Pulisce gli oggetti inattivi
        cleanup_inactive_objects();
        
        // Aggiorna il display
        update_display();
        
        // Controlla condizioni di vittoria/sconfitta
        if(check_win_condition()) {
            game_state.victory = 1;
            game_state.game_over = 1;
        }
        
        usleep(50000); // 50ms di pausa
    }
    
    return NULL;
}

void draw_object(game_object_t* obj) {
    if(!obj->active) return;
    
    pthread_mutex_lock(&screen_mutex);
    
    // Cancella la posizione precedente
    if(obj->old_x != obj->x || obj->old_y != obj->y) {
        clear_object_at_position(obj->old_x, obj->old_y, obj->width, obj->height);
    }
    
    // Disegna l'oggetto nella nuova posizione
    switch(obj->type) {
        case OBJ_FROG:
            draw_frog(obj);
            break;
        case OBJ_CROCODILE:
            draw_crocodile(obj);
            break;
        case OBJ_BULLET:
            draw_bullet(obj);
            break;
        case OBJ_GRENADE:
            draw_grenade(obj);
            break;
        default:
            break;
    }
    
    pthread_mutex_unlock(&screen_mutex);
}

void draw_frog(game_object_t* frog) {
    if(frog->x >= 0 && frog->x < game_state.screen_width - 1 &&
       frog->y >= 0 && frog->y < game_state.screen_height) {
        mvprintw(frog->y, frog->x, "^^");
    }
}

void draw_crocodile(game_object_t* croc) {
    if(croc->y >= 0 && croc->y < game_state.screen_height) {
        for(int i = 0; i < croc->width && croc->x + i < game_state.screen_width; i++) {
            if(croc->x + i >= 0) {
                char croc_char;
                if(i == 0) {
                    croc_char = (croc->dir == DIR_LEFT) ? '<' : '>';
                } else if(i == croc->width - 1) {
                    croc_char = (croc->dir == DIR_LEFT) ? '~' : '~';
                } else {
                    croc_char = '=';
                }
                mvaddch(croc->y, croc->x + i, croc_char);
            }
        }
    }
}

void draw_bullet(game_object_t* bullet) {
    if(bullet->x >= 0 && bullet->x < game_state.screen_width &&
       bullet->y >= 0 && bullet->y < game_state.screen_height) {
        mvaddch(bullet->y, bullet->x, BULLET_CHAR);
    }
}

void draw_grenade(game_object_t* grenade) {
    if(grenade->x >= 0 && grenade->x < game_state.screen_width &&
       grenade->y >= 0 && grenade->y < game_state.screen_height) {
        mvaddch(grenade->y, grenade->x, GRENADE_CHAR);
    }
}

void clear_object_at_position(int x, int y, int width, int height) {
    for(int i = 0; i < width; i++) {
        for(int j = 0; j < height; j++) {
            if(x + i >= 0 && x + i < game_state.screen_width &&
               y + j >= 0 && y + j < game_state.screen_height) {
                
                // Non cancellare elementi statici
                if(!is_static_position(x + i, y + j)) {
                    mvaddch(y + j, x + i, ' ');
                }
            }
        }
    }
}

int is_static_position(int x, int y) {
    // Controlla se la posizione contiene elementi statici
    if(y == game_state.tane_y || 
       y == game_state.grass_y || 
       y == game_state.sidewalk_y ||
       y == game_state.river_start_y - 1 ||
       y == game_state.river_end_y + 1 ||
       y == 0) { // Riga delle informazioni
        return 1;
    }
    return 0;
}

void update_display(void) {
    pthread_mutex_lock(&screen_mutex);
    
    // Ridisegna gli elementi statici
    draw_static_elements();
    
    // Aggiorna le informazioni di gioco
    mvprintw(0, 0, "Vite: %d | Punteggio: %d | Tempo: %d", 
             game_state.lives, game_state.score, game_state.time_left);
    
    // Mostra messaggi di stato se necessario
    if(game_state.game_over) {
        int msg_y = game_state.screen_height / 2;
        int msg_x = game_state.screen_width / 2 - 15;
        
        if(game_state.victory) {
            mvprintw(msg_y, msg_x, "VITTORIA! Punteggio: %d", game_state.score);
            mvprintw(msg_y + 1, msg_x, "Premi 'r' per rigiocare o 'q' per uscire");
        } else {
            mvprintw(msg_y, msg_x, "GAME OVER! Punteggio: %d", game_state.score);
            mvprintw(msg_y + 1, msg_x, "Premi 'r' per rigiocare o 'q' per uscire");
        }
    }
    
    refresh();
    pthread_mutex_unlock(&screen_mutex);
}

int check_win_condition(void) {
    // Controlla se tutte le tane sono chiuse
    for(int i = 0; i < NUM_TANE; i++) {
        if(!game_state.tane_closed[i]) {
            return 0;
        }
    }
    return 1;
}

void handle_game_over_input(void) {
    int ch;
    while((ch = getch()) != ERR) {
        switch(ch) {
            case 'r':
            case 'R':
                restart_game();
                break;
            case 'q':
            case 'Q':
                game_running = 0;
                break;
        }
    }
}

void restart_game(void) {
    // Resetta lo stato del gioco
    game_state.lives = MAX_LIVES;
    game_state.score = 0;
    game_state.time_left = GAME_TIME;
    game_state.game_over = 0;
    game_state.victory = 0;
    
    // Resetta le tane
    for(int i = 0; i < NUM_TANE; i++) {
        game_state.tane_closed[i] = 0;
    }
    
    // Reinizializza le corsie
    init_river_lanes();
    
    // Pulisce lo schermo
    pthread_mutex_lock(&screen_mutex);
    clear();
    pthread_mutex_unlock(&screen_mutex);
    
    // Pulisce la lista degli oggetti
    cleanup_inactive_objects();
}
