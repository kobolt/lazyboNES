CFLAGS=-Wall -Wextra -lSDL2 -lm -lncurses

all: lazyboNES

lazyboNES: main.o cpu.o mem.o ines.o ppu.o apu.o gui.o cli.o tas.o
	gcc -o lazyboNES $^ ${CFLAGS}

main.o: main.c
	gcc -c $^ ${CFLAGS}

cpu.o: cpu.c
	gcc -c $^ ${CFLAGS}

mem.o: mem.c
	gcc -c $^ ${CFLAGS}

ines.o: ines.c
	gcc -c $^ ${CFLAGS}

ppu.o: ppu.c
	gcc -c $^ ${CFLAGS}

apu.o: apu.c
	gcc -c $^ ${CFLAGS}

gui.o: gui.c
	gcc -c $^ ${CFLAGS}

cli.o: cli.c
	gcc -c $^ ${CFLAGS}

tas.o: tas.c
	gcc -c $^ ${CFLAGS}

.PHONY: clean
clean:
	rm -f *.o lazyboNES

