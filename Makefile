help:
	@echo "- help: see this message"
	@echo "- all: build all object files and link to executable to ./bin"

all: bin

bin: main3.c
	gcc main3.c -lpthread -o bin

