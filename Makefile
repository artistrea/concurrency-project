SOURCEDIR = src
HEADERDIR = include
BUILDDIR = build

SOURCES := $(shell find $(SOURCEDIR) -name '*.c')
HEADERS := $(shell find $(HEADERDIR) -name '*.h')
OBJECTS = $(patsubst $(SOURCEDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

CC = gcc
CFLAGS = -Wall
INCLUDE_FLAGS = -lpthread -I$(HEADERDIR)

# debug with gdb:
debug: CFLAGS := -g

debug: all

all: setup bin alt

bin: main3.c
	gcc main3.c -lpthread -o bin

alt: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o alt $(INCLUDE_FLAGS)

setup:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_FLAGS)

help:
	@echo "- help: see this message"
	@echo "- all: build all object files and link to executable to ./bin"
	@echo "- alt: build all object files and link to executable to ./bin"

