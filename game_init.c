#include "frogger.h"

// Variabili globali
game_state_t game_state;
shared_buffer_t shared_buf;
pthread_mutex_t screen_mutex;
int game_running = 1;

void init_game(void) {
    // Inizializza ncurses
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    
    // Ottiene le dimensioni dello schermo
    getmaxyx(stdscr, game_state.screen_height, game_state.screen_width);
    
    // Inizializza lo stato del gioco
    game_state.lives = MAX_LIVES;
    game_state.score = 0;
    game_state.time_left = GAME_TIME;
    game_state.game_over = 0;
    game_state.victory = 0;
    
    // Calcola le posizioni delle aree di gioco
    game_state.tane_y = 1;
    game_state.grass_y = 3;
    game_state.river_start_y = 5;
    game_state.river_end_y = game_state.river_start_y + MIN_RIVER_LANES - 1;
    game_state.sidewalk_y = game_state.screen_height - 3;
    
    // Inizializza le tane (tutte aperte)
    for(int i = 0; i < NUM_TANE; i++) {
        game_state.tane_closed[i] = 0;
    }
    
    // Inizializza le corsie del fiume
    init_river_lanes();
    
    // Inizializza il buffer condiviso
    init_buffer(&shared_buf);
    
    // Inizializza il mutex per lo schermo
    pthread_mutex_init(&screen_mutex, NULL);
    
    // Inizializza il generatore di numeri casuali
    srand(time(NULL));
}

void init_river_lanes(void) {
    for(int i = 0; i < MIN_RIVER_LANES; i++) {
        game_state.lanes[i].lane_num = i;
        game_state.lanes[i].croc_count = 0;
        
        // Alterna le direzioni delle corsie
        if(i == 0) {
            game_state.lanes[i].direction = (rand() % 2) ? DIR_LEFT : DIR_RIGHT;
        } else {
            // Direzione opposta alla corsia precedente
            game_state.lanes[i].direction = (game_state.lanes[i-1].direction == DIR_LEFT) ? DIR_RIGHT : DIR_LEFT;
        }
        
        // Velocità casuale ma compatibile
        game_state.lanes[i].speed = 1 + (rand() % 3); // 1-3
    }
}

void cleanup_game(void) {
    // Pulisce ncurses
    endwin();
    
    // Distrugge il buffer condiviso
    destroy_buffer(&shared_buf);
    
    // Distrugge il mutex dello schermo
    pthread_mutex_destroy(&screen_mutex);
    
    game_running = 0;
}

void reset_round(void) {
    // Resetta il timer
    game_state.time_left = GAME_TIME;
    
    // Reinizializza le corsie del fiume per varietà
    init_river_lanes();
    
    // Pulisce lo schermo
    pthread_mutex_lock(&screen_mutex);
    clear();
    pthread_mutex_unlock(&screen_mutex);
}

int get_random_number(int min, int max) {
    return min + (rand() % (max - min + 1));
}

void draw_static_elements(void) {
    pthread_mutex_lock(&screen_mutex);
    
    // Disegna le tane
    int tana_width = game_state.screen_width / NUM_TANE;
    for(int i = 0; i < NUM_TANE; i++) {
        int tana_x = i * tana_width + tana_width/4;
        if(game_state.tane_closed[i]) {
            mvprintw(game_state.tane_y, tana_x, "[X]");
        } else {
            mvprintw(game_state.tane_y, tana_x, "[ ]");
        }
    }
    
    // Disegna la sponda d'erba
    for(int x = 0; x < game_state.screen_width; x++) {
        mvaddch(game_state.grass_y, x, '~');
    }
    
    // Disegna il marciapiede
    for(int x = 0; x < game_state.screen_width; x++) {
        mvaddch(game_state.sidewalk_y, x, '=');
    }
    
    // Disegna i bordi del fiume
    for(int x = 0; x < game_state.screen_width; x++) {
        mvaddch(game_state.river_start_y - 1, x, '-');
        mvaddch(game_state.river_end_y + 1, x, '-');
    }
    
    // Disegna le informazioni di gioco
    mvprintw(0, 0, "Vite: %d | Punteggio: %d | Tempo: %d", 
             game_state.lives, game_state.score, game_state.time_left);
    
    pthread_mutex_unlock(&screen_mutex);
}
