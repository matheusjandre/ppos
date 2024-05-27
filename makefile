# Matheus Henrique Jandre Ferraz - Bacharelado Ciência da Computação
# GRR: 20215397

PROGRAM = output
MAIN = main.c

DEBUG = ALL
VELOCITY = NORMAL

LIB_DIR = lib
OBJ_DIR = obj
SRC_DIR = src

COMP = gcc
CFLAGS = -Wextra -std=gnu99 -DVELOCITY_$(VELOCITY)

DEPS = queue.o ppos_core.o ppos_ipc.o

all: $(PROGRAM)

debug: CFLAGS += $(foreach flag, $(DEBUG), -DDEBUG_$(flag))
debug: CFLAGS += -include $(LIB_DIR)/dbug/dbug.h
debug: dbug.o $(PROGRAM)

$(PROGRAM): $(DEPS)
	@$(COMP) $(CFLAGS) $(OBJ_DIR)/*.o $(MAIN) -o $@
	@echo "Successfully built. To run: ./$@"
	@echo "Cleaning up object files..."
	@rm -rf $(OBJ_DIR)

# OBJECTS -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

dbug.o: $(LIB_DIR)/dbug/*.h $(LIB_DIR)/dbug/*.c | $(OBJ_DIR)
	@$(COMP) $(CFLAGS) -c $(LIB_DIR)/dbug/*.c -o $(OBJ_DIR)/$@

queue.o: $(LIB_DIR)/queue/*.h $(LIB_DIR)/queue/*.c | $(OBJ_DIR)
	@$(COMP) $(CFLAGS) -c $(LIB_DIR)/queue/*.c -o $(OBJ_DIR)/$@

ppos_core.o: $(SRC_DIR)/ppos.h $(SRC_DIR)/ppos_data.h $(SRC_DIR)/ppos_core.c | $(OBJ_DIR)
	@$(COMP) $(CFLAGS) -c $(SRC_DIR)/ppos_core.c -o $(OBJ_DIR)/$@

ppos_ipc.o: $(SRC_DIR)/ppos.h $(SRC_DIR)/ppos_data.h $(SRC_DIR)/ppos_ipc.c | $(OBJ_DIR)
	@$(COMP) $(CFLAGS) -c $(SRC_DIR)/ppos_ipc.c -o $(OBJ_DIR)/$@

$(OBJ_DIR):
	@mkdir -p $@

# CLEANING -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

clean:
	@rm -rf $(OBJ_DIR) $(PROGRAM) debug
	@echo "Successfully erased compiled files."
