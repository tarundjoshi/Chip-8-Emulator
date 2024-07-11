CFLAGS = -Wall -Werror -Wextra -std=c17

all:
	gcc chip8.c -o chip8 $(CFLAGS) `sdl2-config --cflags --libs`

debug:
	gcc chip8.c -o chip8 $(CFLAGS) `sdl2-config --cflags --libs` -DDEBUG
