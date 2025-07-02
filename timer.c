#include "frogger.h"

void* timer_thread(void* arg) {
    while(game_running) {
        if(!game_state.game_over && game_state.time_left > 0) {
            // Decrementa il tempo ogni secondo
            sleep(1);
            game_state.time_left--;
            
            // Controlla se il tempo è finito
            if(game_state.time_left <= 0) {
                handle_time_up();
            }
            
            // Aggiorna il display del tempo
            update_time_display();
        } else {
            // Se il gioco è finito o in pausa, attende
            sleep(1);
        }
    }
    
    return NULL;
}

void handle_time_up(void) {
    // Il tempo è finito - la rana perde una vita
    lose_life();
    
    // Mostra messaggio temporaneo
    show_time_up_message();
}

void update_time_display(void) {
    pthread_mutex_lock(&screen_mutex);
    
    // Aggiorna solo la parte del tempo per efficienza
    mvprintw(0, 35, "Tempo: %d ", game_state.time_left);
    
    // Cambia colore se il tempo sta per finire
    if(game_state.time_left <= 10) {
        // Effetto lampeggiante negli ultimi 10 secondi
        if(game_state.time_left % 2 == 0) {
            mvprintw(0, 35, "Tempo: %d!!", game_state.time_left);
        }
    }
    
    refresh();
    pthread_mutex_unlock(&screen_mutex);
}

void show_time_up_message(void) {
    pthread_mutex_lock(&screen_mutex);
    
    int msg_y = game_state.screen_height / 2;
    int msg_x = game_state.screen_width / 2 - 10;
    
    mvprintw(msg_y, msg_x, "TEMPO SCADUTO!");
    refresh();
    
    pthread_mutex_unlock(&screen_mutex);
    
    // Mostra il messaggio per 2 secondi
    sleep(2);
}

void pause_timer(void) {
    // Questa funzione può essere usata per implementare la pausa
    // Per ora non implementata completamente
}

void resume_timer(void) {
    // Questa funzione può essere usata per riprendere da pausa
    // Per ora non implementata completamente
}

void reset_timer(void) {
    game_state.time_left = GAME_TIME;
}

int get_time_bonus(void) {
    // Calcola il bonus basato sul tempo rimanente
    return game_state.time_left * 2;
}

void add_time_bonus(void) {
    // Aggiunge tempo bonus in determinate condizioni
    // Ad esempio quando si chiude una tana
    if(game_state.time_left < GAME_TIME - 10) {
        game_state.time_left += 5;
        show_time_bonus_message();
    }
}

void show_time_bonus_message(void) {
    pthread_mutex_lock(&screen_mutex);
    
    int msg_y = game_state.screen_height / 2 + 2;
    int msg_x = game_state.screen_width / 2 - 8;
    
    mvprintw(msg_y, msg_x, "+5 SECONDI BONUS!");
    refresh();
    
    pthread_mutex_unlock(&screen_mutex);
    
    // Mostra il messaggio per 1 secondo
    sleep(1);
    
    // Cancella il messaggio
    pthread_mutex_lock(&screen_mutex);
    for(int i = 0; i < 17; i++) {
        mvaddch(msg_y, msg_x + i, ' ');
    }
    refresh();
    pthread_mutex_unlock(&screen_mutex);
}
