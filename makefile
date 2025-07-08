
CC=gcc

SRCDIR = src
BINDIR = bin

OUTPUT = $(BINDIR)/bober

# X11 stuff
X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

CFLAGS = -Wall -Wextra

INCS = -I$(X11INC) -I/usr/include/freetype2
LIBS = -L$(X11LIB) -lX11 -lvterm -lXft -lfreetype -util

all: clean make_bindir build run

make_bindir:
	mkdir -p $(BINDIR)

build:
	$(CC) $(INCS) $(CFLAGS) $(SRCDIR)/*.c -o $(OUTPUT) $(LIBS)

run:
	./$(OUTPUT)

clean:
	rm -rf $(BINDIR)
