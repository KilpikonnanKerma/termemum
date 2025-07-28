
ASM=nasm
CC=gcc

# X11 stuff
X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

CFLAGS = -f elf64

INCS = -I$(X11INC) -I/usr/include/freetype2
LDLIBS = -L$(X11LIB) -lX11 -lvterm -lXft -lfreetype -util

all: build

build:
	$(ASM) $(CFLAGS) main.asm -o main.o
	$(CC) main.o -o main $(LDLIBS)

run:
	./$(OUTPUT)

clean:
	rm -rf $(BINDIR)
	@if [ -f $(DESKTOP_FILE) ]; then \
		rm $(DESKTOP_FILE); \
	fi