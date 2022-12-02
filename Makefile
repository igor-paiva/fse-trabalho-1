# Executable
BINFOLDER := bin/
# .h
INCFOLDER := inc/
# .c
SRCFOLDER := src/
# .o
OBJFOLDER := obj/

# Compiler
CC := gcc

CFLAGS := -g -W -Wall -Wextra -pedantic

SRCFILES := $(wildcard src/*.c)

all: $(SRCFILES:src/%.c=obj/%.o)
	$(CC) $(OBJFOLDER)*.o $(CFLAGS) -o $(BINFOLDER)prog

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I ./$(INCFOLDER)

run: $(BINFOLDER)
	./$(BINFOLDER)prog

.PHONY: clean

clean:
	rm -rf obj/*
	rm -rf bin/*
