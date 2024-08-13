# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11

# SDL2 flags
SDL_CFLAGS = $(shell sdl2-config --cflags) 
SDL_LDFLAGS = $(shell sdl2-config --libs) -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lm

# Source files
SRC = $(wildcard  *.c)

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
EXEC = space

# Default target
all: $(EXEC)

# Linking
$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $@ $(SDL_LDFLAGS)

# Compilation
%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJ) $(EXEC)

# Phony targets
.PHONY: all clean
