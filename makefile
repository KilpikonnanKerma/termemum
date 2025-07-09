
CC=gcc

SRCDIR = src
BINDIR = bin

OUTPUT = $(BINDIR)/termemum
INSTALL_NAME = termemum
INSTALL_PATH = /usr/local/bin

TEMP_DIR = tmp
DESKTOP_FILE = termemum.desktop
DESKTOP_PATH_USER = $(HOME)/.local/share/applications

# X11 stuff
X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

CFLAGS = -Wall -Wextra

INCS = -I$(X11INC) -I/usr/include/freetype2
LIBS = -L$(X11LIB) -lX11 -lvterm -lXft -lfreetype -util

all: clean build run

make_direcories:
	mkdir -p $(BINDIR)
	mkdir -p $(TEMP_DIR)

build: make_direcories
	$(CC) $(INCS) $(CFLAGS) $(SRCDIR)/*.c -o $(OUTPUT) $(LIBS)

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
