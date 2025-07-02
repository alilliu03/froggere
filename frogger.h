#ifndef FROGGER_H
#define FROGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

// Costanti di gioco
#define MAX_LIVES 3
#define NUM_TANE 5
#define MIN_RIVER_LANES 8
#define FROG_WIDTH 2
#define FROG_HEIGHT 1
#define CROC_MIN_WIDTH 4
#define CROC_MAX_WIDTH 6
#define CROC_HEIGHT 1
#define BULLET_CHAR '*'
#define GRENADE_CHAR 'o'
#define GAME_TIME 60
#define BUFFER_SIZE 100

// Direzioni
typedef enum {
    DIR_UP = 0,
    DIR_DOWN = 1,
    DIR_LEFT = 2,
    DIR_RIGHT = 3,
    DIR_NONE = 4
} direction_t;

// Tipi di oggetti
typedef enum {
    OBJ_FROG = 0,
    OBJ_CROCODILE = 1,
    OBJ_BULLET = 2,
    OBJ_GRENADE = 3,
    OBJ_COLLISION = 4,
    OBJ_DESTROY = 5
} obj_type_t;

// Struttura per le coordinate
typedef struct {
    int x, y;
    int old_x, old_y;
    obj_type_t type;
    int id;
    direction_t dir;
    int active;
    int lane;
    int width, height;
} game_object_t;

// Struttura per le corsie del fiume
typedef struct {
    int lane_num;
    direction_t direction;
    int speed;
    int croc_count;
} river_lane_t;

// Struttura per lo stato del gioco
typedef struct {
    int lives;
    int score;
    int time_left;
    int tane_closed[NUM_TANE];
    int game_over;
    int victory;
    int screen_width, screen_height;
    int river_start_y, river_end_y;
    int grass_y, sidewalk_y;
    int tane_y;
    river_lane_t lanes[MIN_RIVER_LANES];
} game_state_t;

// Buffer per comunicazione thread
typedef struct {
    game_object_t buffer[BUFFER_SIZE];
    int head, tail, count;
    pthread_mutex_t mutex;
    sem_t empty, full;
} shared_buffer_t;

// Variabili globali
extern game_state_t game_state;
extern shared_buffer_t shared_buf;
extern pthread_mutex_t screen_mutex;
extern int game_running;

// Prototipi delle funzioni principali
void init_game(void);
void cleanup_game(void);
void draw_game_area(void);
void update_display(void);
void check_collisions(void);
int check_win_condition(void);
void reset_round(void);

// Prototipi per la gestione degli oggetti
void* frog_thread(void* arg);
void* crocodile_thread(void* arg);
void* bullet_thread(void* arg);
void* grenade_thread(void* arg);
void* game_manager_thread(void* arg);
void* timer_thread(void* arg);

// Prototipi per il buffer condiviso
void init_buffer(shared_buffer_t* buf);
void destroy_buffer(shared_buffer_t* buf);
int put_object(shared_buffer_t* buf, game_object_t obj);
int get_object(shared_buffer_t* buf, game_object_t* obj);

// Prototipi utilità
int is_valid_position(int x, int y, int width, int height);
int objects_collide(game_object_t* obj1, game_object_t* obj2);
void generate_crocodiles(void);
int get_random_number(int min, int max);

#endif // FROGGER_H
