SOURCEDIR = src
HEADERDIR = include
BUILDDIR = build
EXAMPLESDIR = examples

SOURCES := $(shell find $(SOURCEDIR) -name '*.c')
HEADERS := $(shell find $(HEADERDIR) -name '*.h')
OBJECTS = $(patsubst $(SOURCEDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))
EXAMPLES := $(shell find $(EXAMPLESDIR) -name '*.c')
EXAMPLESOUT = $(patsubst $(EXAMPLESDIR)/%.c, %.out, $(EXAMPLES))

CC = gcc
CFLAGS = -Wall
INCLUDE_FLAGS = -lpthread -I$(HEADERDIR)

# debug with gdb:
debug: CFLAGS := -g

debug: all

all: setup $(EXAMPLESOUT)

# bin: $(OBJECTS)
# 	$(CC) $(CFLAGS) $(OBJECTS) -o bin $(INCLUDE_FLAGS)

%.out: $(BUILDDIR)/%.o $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $< -o $@ $(INCLUDE_FLAGS)

setup:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_FLAGS)

$(BUILDDIR)/%.o: $(EXAMPLESDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_FLAGS)

clean:
	rm -rf build
	rm -f bin

help:
	@echo "- help: see this message"
	@echo "- all: setup build folder and run 'bin'"
	@echo "- bin: build all object files to '/build' and link to executable to ./bin"
	@echo "- clean: remove all object files from '/build' and removes executable 'bin'"

