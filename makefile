# Matheus Henrique Jandre Ferraz - Bacharelado Ciência da Computação
# GRR: 20215397

COMP = gcc
CFLAGS = -Wall -g -std=c99

PROGRAM = output
MAIN = main.c

LIB = lib
OBJ = obj
SRC = src

DEP = queue.o ppos.o

all: $(DEP)
	@$(COMP) $(CFLAGS) $(OBJ)/*.o $(MAIN) -o $(PROGRAM)
	@echo "Successfully build. To run: ./$(PROGRAM)"

debug: CFLAGS += -DDEBUG
debug: $(DEP)
	@$(COMP) $(CFLAGS) $(OBJ)/*.o $(MAIN) -o debug
	@echo "[DEBUG] Successfully build. To run: ./debug"

# OBJECTS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

queue.o: $(LIB)/queue/queue.h
	@$(COMP) $(CFLAGS) -c $(LIB)/queue/queue.c -o $(OBJ)/queue.o

ppos.o: $(LIB)/ppos/ppos.h $(LIB)/ppos/ppos_data.h
	@$(COMP) $(CFLAGS) -c $(LIB)/ppos/ppos_core.c -o $(OBJ)/ppos_core.o

# CLEANING -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

clean:
	@rm -rf $(OBJ)/*.o $(PROGRAM) debug
	@echo "Successfully purged files."
