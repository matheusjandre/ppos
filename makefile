# Matheus Henrique Jandre Ferraz - Bacharelado Ciência da Computação
# GRR: 20215397

COMP = gcc
CFLAGS = -Wall -std=c99

PROGRAM = output
MAIN = testafila.c

LIB = lib
OBJ = obj
SRC = src

DEP = queue.o

all: $(DEP)
	@$(COMP) $(CFLAGS) $(OBJ)/*.o $(MAIN) -o $(PROGRAM)
	@echo "Successfully build. To run: ./$(PROGRAM)"

# OBJECTS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

queue.o: $(LIB)/queue/queue.h
	@$(COMP) $(CFLAGS) -c $(LIB)/queue/queue.c -o $(OBJ)/queue.o

# CLEANING -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

clean:
	@rm -rf *.o $(PROGRAM)
	@echo "Successfully purged files."
