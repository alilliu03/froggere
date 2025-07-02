CC = gcc
CFLAGS = -Wall -Wextra -g -c
LDFLAGS = -lncursesw

froggere.exe: main.o buffer.o collision.o crocodile.o frog.c game_init.o game_manager.o timer.o utils.o
	$(CC) main.o buffer.o collision.o crocodile.o frog.c game_init.o game_manager.o timer.o utils.o $(LDFLAGS) -o froggere.exe

main.o: main.c frogger.h
	$(CC) $(CFLAGS) main.c 

buffer.o: buffer.c frogger.h
	$(CC) $(CFLAGS) buffer.c
	
collision.o: collision.c frogger.h 
	$(CC) $(CFLAGS) collision.c

crocodile.o: crocodile.c frogger.h
	$(CC) $(CFLAGS) crocodile.c

frog.o: frog.c frogger.h
	$(CC) $(CFLAGS) frog.c

game_init.o: game_init.c frogger.h
	$(CC) $(CFLAGS) game_init.c

 game_manager.o: game_manager.c frogger.h
	$(CC) $(CFLAGS) gamr_manager.c

 timer.o: timer.c frogger.h
	$(CC) $(CFLAGS) timer.c

 utils.o: utils.c frogger.h
	$(CC) $(CFLAGS) utils.c
