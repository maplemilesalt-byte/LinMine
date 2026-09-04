CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_TTF_CFLAGS := $(shell pkg-config --cflags SDL2_ttf 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)
SDL_TTF_LIBS := $(shell pkg-config --libs SDL2_ttf 2>/dev/null)

TARGET := linmine
SOURCES := main.c gfx.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) gfx.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS) -I. -o $@ $(SOURCES) $(SDL_LIBS) $(SDL_TTF_LIBS)

clean:
	rm -f $(TARGET)
