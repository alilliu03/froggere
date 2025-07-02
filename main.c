#include "frogger.h"

// Variabili globali
game_state_t game_state;
shared_buffer_t shared_buf;
pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;
int game_running = 1;

void init_game(void) {
    // Inizializza ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    // Inizializza colori se supportati
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Rana
        init_pair(2, COLOR_YELLOW, COLOR_BLUE);   // Coccodrilli
        init_pair(3, COLOR_RED, COLOR_BLACK);     // Proiettili
        init_pair(4, COLOR_WHITE, COLOR_GREEN);   // Prato
        init_pair(5, COLOR_BLUE, COLOR_BLACK);    // Acqua
        init_pair(6, COLOR_CYAN, COLOR_BLACK);    // Granate
        init_pair(7, COLOR_MAGENTA, COLOR_BLACK); // Tane
    }
    
    // Ottieni dimensioni schermo
    getmaxyx(stdscr, game_state.screen_height, game_state.screen_width);
    
    // Inizializza stato del gioco
    game_state.lives = MAX_LIVES;
    game_state.score = 0;
    game_state.time_left = GAME_TIME;
    game_state.game_over = 0;
    game_state.victory = 0;
    
    // Calcola posizioni aree di gioco
    game_state.tane_y = 1;
    game_state.grass_y = 3;
    game_state.river_start_y = 5;
    game_state.river_end_y = game_state.river_start_y + MIN_RIVER_LANES;
    game_state.sidewalk_y = game_state.screen_height - 3;
    
    // Inizializza tane
    for (int i = 0; i < NUM_TANE; i++) {
        game_state.tane_closed[i] = 0;
    }
    
    // Inizializza corsie del fiume
    for (int i = 0; i < MIN_RIVER_LANES; i++) {
        game_state.lanes[i].lane_num = i;
        game_state.lanes[i].direction = (i % 2 == 0) ? DIR_RIGHT : DIR_LEFT;
        game_state.lanes[i].speed = get_random_number(1, 3);
        game_state.lanes[i].croc_count = 0;
    }
    
    // Inizializza buffer condiviso
    init_buffer(&shared_buf);
    
    srand(time(NULL));
}

void cleanup_game(void) {
    game_running = 0;
    destroy_buffer(&shared_buf);
    pthread_mutex_destroy(&screen_mutex);
    endwin();
}

void draw_game_area(void) {
    pthread_mutex_lock(&screen_mutex);
    
    clear();
    
    // Disegna tane
    attron(COLOR_PAIR(7));
    for (int i = 0; i < NUM_TANE; i++) {
        int tana_x = (game_state.screen_width / (NUM_TANE + 1)) * (i + 1) - 2;
        if (!game_state.tane_closed[i]) {
            mvprintw(game_state.tane_y, tana_x, "[  ]");
        } else {
            mvprintw(game_state.tane_y, tana_x, "[XX]");
        }
    }
    attroff(COLOR_PAIR(7));
    
    // Disegna prato
    attron(COLOR_PAIR(4));
    for (int x = 0; x < game_state.screen_width; x++) {
        mvaddch(game_state.grass_y, x, '~');
    }
    attroff(COLOR_PAIR(4));
    
    // Disegna fiume
    attron(COLOR_PAIR(5));
    for (int y = game_state.river_start_y; y < game_state.river_end_y; y++) {
        for (int x = 0; x < game_state.screen_width; x++) {
            mvaddch(y, x, '~');
        }
    }
    attroff(COLOR_PAIR(5));
    
    // Disegna marciapiede
    for (int x = 0; x < game_state.screen_width; x++) {
        mvaddch(game_state.sidewalk_y, x, '=');
    }
    
    // Disegna informazioni di gioco
    mvprintw(0, 0, "Vite: %d  Score: %d  Tempo: %d", 
             game_state.lives, game_state.score, game_state.time_left);
    
    pthread_mutex_unlock(&screen_mutex);
}

void update_display(void) {
    pthread_mutex_lock(&screen_mutex);
    refresh();
    pthread_mutex_unlock(&screen_mutex);
}

void check_collisions(void) {
    // Implementazione delle collisioni gestita nel game_manager_thread
}

int check_win_condition(void) {
    for (int i = 0; i < NUM_TANE; i++) {
        if (!game_state.tane_closed[i]) {
            return 0;
        }
    }
    return 1;
}

void reset_round(void) {
    game_state.time_left = GAME_TIME;
    // Reset delle posizioni gestito dai singoli thread
}

void* frog_thread(void* arg) {
    game_object_t frog;
    frog.type = OBJ_FROG;
    frog.id = 0;
    frog.width = FROG_WIDTH;
    frog.height = FROG_HEIGHT;
    frog.x = game_state.screen_width / 2;
    frog.y = game_state.sidewalk_y - 1;
    frog.active = 1;
    frog.dir = DIR_NONE;
    
    int ch;
    
    while (game_running && !game_state.game_over) {
        frog.old_x = frog.x;
        frog.old_y = frog.y;
        
        ch = getch();
        
        switch (ch) {
            case KEY_UP:
                if (frog.y - FROG_HEIGHT >= game_state.tane_y) {
                    frog.y -= FROG_HEIGHT;
                    frog.dir = DIR_UP;
                }
                break;
            case KEY_DOWN:
                if (frog.y + FROG_HEIGHT < game_state.sidewalk_y) {
                    frog.y += FROG_HEIGHT;
                    frog.dir = DIR_DOWN;
                }
                break;
            case KEY_LEFT:
                if (frog.x - FROG_WIDTH >= 0) {
                    frog.x -= FROG_WIDTH;
                    frog.dir = DIR_LEFT;
                }
                break;
            case KEY_RIGHT:
                if (frog.x + FROG_WIDTH < game_state.screen_width) {
                    frog.x += FROG_WIDTH;
                    frog.dir = DIR_RIGHT;
                }
                break;
            case ' ':
                // Spara granate
                {
                    game_object_t grenade_left, grenade_right;
                    
                    grenade_left.type = OBJ_GRENADE;
                    grenade_left.x = frog.x;
                    grenade_left.y = frog.y;
                    grenade_left.dir = DIR_LEFT;
                    grenade_left.active = 1;
                    grenade_left.width = 1;
                    grenade_left.height = 1;
                    grenade_left.id = rand();
                    
                    grenade_right = grenade_left;
                    grenade_right.dir = DIR_RIGHT;
                    grenade_right.id = rand();
                    
                    put_object(&shared_buf, grenade_left);
                    put_object(&shared_buf, grenade_right);
                }
                break;
            case 'q':
                game_running = 0;
                break;
        }
        
        put_object(&shared_buf, frog);
        usleep(50000); // 50ms
    }
    
    return NULL;
}

void* crocodile_thread(void* arg) {
    int lane = *((int*)arg);
    game_object_t croc;
    
    croc.type = OBJ_CROCODILE;
    croc.id = rand();
    croc.lane = lane;
    croc.width = get_random_number(CROC_MIN_WIDTH, CROC_MAX_WIDTH);
    croc.height = CROC_HEIGHT;
    croc.y = game_state.river_start_y + lane;
    croc.dir = game_state.lanes[lane].direction;
    croc.active = 1;
    
    if (croc.dir == DIR_RIGHT) {
        croc.x = -croc.width;
    } else {
        croc.x = game_state.screen_width;
    }
    
    int bullet_counter = 0;
    
    while (game_running && !game_state.game_over) {
        croc.old_x = croc.x;
        croc.old_y = croc.y;
        
        // Movimento
        if (croc.dir == DIR_RIGHT) {
            croc.x += game_state.lanes[lane].speed;
            if (croc.x > game_state.screen_width) {
                break; // Coccodrillo uscito dallo schermo
            }
        } else {
            croc.x -= game_state.lanes[lane].speed;
            if (croc.x + croc.width < 0) {
                break; // Coccodrillo uscito dallo schermo
            }
        }
        
        // Sparo casuale
        bullet_counter++;
        if (bullet_counter > 50 && get_random_number(0, 100) < 5) {
            game_object_t bullet;
            bullet.type = OBJ_BULLET;
            bullet.x = croc.x + croc.width / 2;
            bullet.y = croc.y;
            bullet.dir = croc.dir;
            bullet.active = 1;
            bullet.width = 1;
            bullet.height = 1;
            bullet.id = rand();
            
            put_object(&shared_buf, bullet);
            bullet_counter = 0;
        }
        
        put_object(&shared_buf, croc);
        usleep(100000); // 100ms
    }
    
    return NULL;
}

void* bullet_thread(void* arg) {
    game_object_t* bullet_data = (game_object_t*)arg;
    game_object_t bullet = *bullet_data;
    
    while (game_running && !game_state.game_over && bullet.active) {
        bullet.old_x = bullet.x;
        bullet.old_y = bullet.y;
        
        if (bullet.dir == DIR_RIGHT) {
            bullet.x += 2;
        } else {
            bullet.x -= 2;
        }
        
        if (bullet.x < 0 || bullet.x >= game_state.screen_width) {
            bullet.active = 0;
            break;
        }
        
        put_object(&shared_buf, bullet);
        usleep(80000); // 80ms
    }
    
    return NULL;
}

void* grenade_thread(void* arg) {
    game_object_t* grenade_data = (game_object_t*)arg;
    game_object_t grenade = *grenade_data;
    
    while (game_running && !game_state.game_over && grenade.active) {
        grenade.old_x = grenade.x;
        grenade.old_y = grenade.y;
        
        if (grenade.dir == DIR_RIGHT) {
            grenade.x += 2;
        } else {
            grenade.x -= 2;
        }
        
        if (grenade.x < 0 || grenade.x >= game_state.screen_width) {
            grenade.active = 0;
            break;
        }
        
        put_object(&shared_buf, grenade);
        usleep(80000); // 80ms
    }
    
    return NULL;
}

void* game_manager_thread(void* arg) {
    game_object_t objects[1000];
    int obj_count = 0;
    
    while (game_running && !game_state.game_over) {
        game_object_t obj;
        
        // Leggi oggetti dal buffer
        while (get_object(&shared_buf, &obj)) {
            if (obj.active) {
                // Trova o aggiungi oggetto
                int found = 0;
                for (int i = 0; i < obj_count; i++) {
                    if (objects[i].id == obj.id && objects[i].type == obj.type) {
                        objects[i] = obj;
                        found = 1;
                        break;
                    }
                }
                
                if (!found && obj_count < 1000) {
                    objects[obj_count++] = obj;
                }
            }
        }
        
        // Disegna area di gioco
        draw_game_area();
        
        // Disegna oggetti
        pthread_mutex_lock(&screen_mutex);
        for (int i = 0; i < obj_count; i++) {
            if (objects[i].active) {
                // Cancella vecchia posizione
                if (objects[i].old_x != objects[i].x || objects[i].old_y != objects[i].y) {
                    for (int h = 0; h < objects[i].height; h++) {
                        for (int w = 0; w < objects[i].width; w++) {
                            if (objects[i].old_y + h >= 0 && objects[i].old_y + h < game_state.screen_height &&
                                objects[i].old_x + w >= 0 && objects[i].old_x + w < game_state.screen_width) {
                                mvaddch(objects[i].old_y + h, objects[i].old_x + w, ' ');
                            }
                        }
                    }
                }
                
                // Disegna nuova posizione
                switch (objects[i].type) {
                    case OBJ_FROG:
                        attron(COLOR_PAIR(1));
                        mvprintw(objects[i].y, objects[i].x, "@@");
                        attroff(COLOR_PAIR(1));
                        break;
                    case OBJ_CROCODILE:
                        attron(COLOR_PAIR(2));
                        for (int w = 0; w < objects[i].width; w++) {
                            mvaddch(objects[i].y, objects[i].x + w, '#');
                        }
                        attroff(COLOR_PAIR(2));
                        break;
                    case OBJ_BULLET:
                        attron(COLOR_PAIR(3));
                        mvaddch(objects[i].y, objects[i].x, BULLET_CHAR);
                        attroff(COLOR_PAIR(3));
                        break;
                    case OBJ_GRENADE:
                        attron(COLOR_PAIR(6));
                        mvaddch(objects[i].y, objects[i].x, GRENADE_CHAR);
                        attroff(COLOR_PAIR(6));
                        break;
                }
            }
        }
        pthread_mutex_unlock(&screen_mutex);
        
        // Controlla collisioni e condizioni di gioco
        // [Implementazione semplificata delle collisioni]
        
        update_display();
        usleep(50000); // 50ms
    }
    
    return NULL;
}

void* timer_thread(void* arg) {
    while (game_running && !game_state.game_over && game_state.time_left > 0) {
        sleep(1);
        game_state.time_left--;
        
        if (game_state.time_left <= 0) {
            game_state.lives--;
            if (game_state.lives <= 0) {
                game_state.game_over = 1;
            } else {
                reset_round();
            }
        }
    }
    
    return NULL;
}

void generate_crocodiles(void) {
    for (int i = 0; i < MIN_RIVER_LANES; i++) {
        if (game_state.lanes[i].croc_count < 3) {
            pthread_t croc_thread;
            int* lane_num = malloc(sizeof(int));
            *lane_num = i;
            
            if (pthread_create(&croc_thread, NULL, crocodile_thread, lane_num) == 0) {
                pthread_detach(croc_thread);
                game_state.lanes[i].croc_count++;
            }
        }
    }
}

int main() {
    pthread_t frog_tid, manager_tid, timer_tid;
    
    init_game();
    
    // Crea thread principali
    pthread_create(&frog_tid, NULL, frog_thread, NULL);
    pthread_create(&manager_tid, NULL, game_manager_thread, NULL);
    pthread_create(&timer_tid, NULL, timer_thread, NULL);
    
    // Genera coccodrilli periodicamente
    while (game_running && !game_state.game_over) {
        generate_crocodiles();
        sleep(2);
    }
    
    // Attendi terminazione thread
    pthread_join(frog_tid, NULL);
    pthread_join(manager_tid, NULL);
    pthread_join(timer_tid, NULL);
    
    // Messaggio finale
    pthread_mutex_lock(&screen_mutex);
    clear();
    if (game_state.victory) {
        mvprintw(game_state.screen_height/2, game_state.screen_width/2 - 10, "VITTORIA! Score: %d", game_state.score);
    } else {
        mvprintw(game_state.screen_height/2, game_state.screen_width/2 - 10, "GAME OVER! Score: %d", game_state.score);
    }
    mvprintw(game_state.screen_height/2 + 2, game_state.screen_width/2 - 15, "Premi un tasto per uscire...");
    refresh();
    pthread_mutex_unlock(&screen_mutex);
    
    nodelay(stdscr, FALSE);
    getch();
    
    cleanup_game();
    return 0;
}
