
CC=gcc
ASM=nasm

SRCDIR = src
BINDIR = bin

OUTPUT = $(BINDIR)/termemum
TEMP_DIR = tmp
DESKTOP_FILE = termemum.desktop
DESKTOP_PATH_USER = $(HOME)/.local/share/applications

C_SRCS := $(wildcard $(SRCDIR)/*.c)
C_OBJS := $(patsubst $(SRCDIR)/%.c,$(TEMP_DIR)/%.o,$(C_SRCS))
ASM_OBS := $(TEMP_DIR)/main.o

INSTALL_NAME = termemum
INSTALL_PATH = /usr/local/bin

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

CFLAGS = -Wall -Wextra
ASM_FLAGS = -f elf64

INCS = -I$(X11INC) -I/usr/include/freetype2 -I./ -I$(SRCDIR)
LIBS = -L$(X11LIB) -lX11 -lvterm -lXft -lfreetype -util

all: clean make_directories build run

make_directories:
	mkdir -p $(BINDIR)
	mkdir -p $(TEMP_DIR)

$(TEMP_DIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCS)

$(ASM_OBJ): $(SRCDIR)/main.asm
	$(ASM) $(ASM_FLAGS) $< -o $@

$(OUTPUT): $(ASM_OBJ) $(C_OBJS)
	$(CC) -nostdlib -no-pie -Wl,--build-id=none $^ -o $@ $(LIBS)

build: $(OUTPUT)

run:
	./$(OUTPUT)

clean:
	rm -rf $(BINDIR)
	@if [ -f $(DESKTOP_FILE) ]; then \
		rm $(DESKTOP_FILE); \
	fi

desktop-file:
	@echo "[Desktop Entry]" > $(TEMP_DIR)/$(DESKTOP_FILE)
	@echo "Name=Termemum" >> $(TEMP_DIR)/$(DESKTOP_FILE)
	@echo "Comment=Terminal emulator handcrafted in C, blessed by X11 and pain" >> $(TEMP_DIR)/$(DESKTOP_FILE)
	@echo "Exec=$(INSTALL_NAME)" >> $(TEMP_DIR)/$(DESKTOP_FILE)
	@echo "Icon=utilities-terminal" >> $(TEMP_DIR)/$(DESKTOP_FILE)
	@echo "Terminal=false" >> $(TEMP_DIR)/$(DESKTOP_FILE)
	@echo "Type=Application" >> $(TEMP_DIR)/$(DESKTOP_FILE)
	@echo "Categories=Utility;TerminalEmulator;" >> $(TEMP_DIR)/$(DESKTOP_FILE)

install: clean build desktop-file
	sudo install -Dm755 $(OUTPUT) $(INSTALL_PATH)/$(INSTALL_NAME)

	mkdir -p $(DESKTOP_PATH_USER)
	cp $(TEMP_DIR)/$(DESKTOP_FILE) $(DESKTOP_PATH_USER)/
	chmod +x $(DESKTOP_PATH_USER)/$(DESKTOP_FILE)

	@echo "Installed $(INSTALL_NAME) to $(INSTALL_PATH) for this user."

uninstall:
	sudo rm -f $(INSTALL_PATH)/$(INSTALL_NAME)

	@if [ -f $(DESKTOP_PATH_USER)/$(DESKTOP_FILE) ]; then \
		echo "Removing .desktop file from user applications"; \
		rm $(DESKTOP_PATH_USER)/$(DESKTOP_FILE); \
	else \
		echo ".desktop file not found, nothing to uninstall"; \
	fi

	@echo "$(INSTALL_NAME) has been uninstalled from $(INSTALL_PATH)"
