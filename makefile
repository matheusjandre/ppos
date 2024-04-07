# Matheus Henrique Jandre Ferraz - Bacharelado Ciência da Computação
# GRR: 20215397

PROGRAM = output
MAIN = main.c

LIB_DIR = lib
OBJ_DIR = obj
SRC_DIR = src

COMP = gcc
CFLAGS = -Wextra -std=gnu99

DEPS = queue.o ppos.o

all: $(PROGRAM)

debug: CFLAGS += -DDEBUG
debug: $(PROGRAM)

$(PROGRAM): $(DEPS)
	@$(COMP) $(CFLAGS) $(OBJ_DIR)/*.o $(MAIN) -o $@
	@echo "Successfully built. To run: ./$@"
	@echo "Cleaning up object files..."
	@rm -rf $(OBJ_DIR)

# OBJECTS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

queue.o: $(LIB_DIR)/*/queue.h $(LIB_DIR)/*/queue.c | $(OBJ_DIR)
	@$(COMP) $(CFLAGS) -c $(LIB_DIR)/*/queue.c -o $(OBJ_DIR)/$@

ppos.o: $(SRC_DIR)/ppos.h $(SRC_DIR)/ppos_data.h $(SRC_DIR)/ppos_core.c | $(OBJ_DIR)
	@$(COMP) $(CFLAGS) -c $(SRC_DIR)/ppos*.c -o $(OBJ_DIR)/$@

$(OBJ_DIR):
	@mkdir -p $@

# CLEANING -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

clean:
	@rm -rf $(OBJ_DIR) $(PROGRAM) debug
	@echo "Successfully purged files."
