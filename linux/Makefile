CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 2>/dev/null)

TARGET := linmine
SOURCES := main.c gfx.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) gfx.h
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -I. -o $@ $(SOURCES) $(GTK_LIBS)

clean:
	rm -f $(TARGET)
