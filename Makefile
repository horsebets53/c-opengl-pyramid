# Compiler settings
CC = gcc
CFLAGS = -Wall -O2 -I include

# Detect OS
UNAME_S := $(shell uname -s)

# Libraries
LIBS = -lglfw -lm

# OS Specific flags
ifeq ($(UNAME_S), Linux)
    LIBS += -lGL -ldl
endif

ifeq ($(UNAME_S), Darwin)
    LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
endif

# Windows (MinGW) - assuming libs are in 'lib' folder
ifeq ($(OS), Windows_NT)
    LIBS += -L lib -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
endif

# Source files
SRC = main.c glad.c
OBJ = $(SRC:.c=.o)
EXEC = pyramid

# Build rules
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
	rm -f pyramid.exe