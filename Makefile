# Executable
BINFOLDER := bin
# .h
INCFOLDER := inc
# .c
SRCFOLDER := src
# .o
OBJFOLDER := obj

# Address to run the server
ADDRESS := localhost

# Port to run the server
PORT := 5000

# Compiler
CC := gcc

CFLAGS := -g -W -Wall -Wextra -pedantic -lpthread # -lwiringPi

SRCFILES := $(wildcard $(SRCFOLDER)/*.c)

all: $(SRCFILES:$(SRCFOLDER)/%.c=$(OBJFOLDER)/%.o)
	$(CC) $(OBJFOLDER)/*.o $(CFLAGS) -o $(BINFOLDER)/prog

$(OBJFOLDER)/%.o: $(SRCFOLDER)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I ./$(INCFOLDER)/

run:
	@./$(BINFOLDER)/prog $(ADDRESS) $(PORT)

.PHONY: clean

clean:
	rm -rf $(OBJFOLDER)/*.*
	rm -rf $(BINFOLDER)/*
