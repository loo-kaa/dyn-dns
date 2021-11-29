CC=gcc
CFLAGS_DEBUG=-g -Wall -Og
CFLAGS_BUILD=-O2

SRC_DIR=src
LIB_DIR=src/lib

LIBS=$(LIB_DIR)/hashmap.o $(LIB_DIR)/logger.o $(SRC_DIR)/mlib.o $(SRC_DIR)/utils.o

LIBRARIES=-lcurl -pthread -lsystemd

BIN=dyn-dns

# all build flags declaration
all: CFLAGS=$(CFLAGS_BUILD)
all: $(BIN)

# lib folder compile
%.o: $(LIB_DIR)/%.h $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ $(LIBRARIES)

# src folder compile
%.o: $(LIBS) $(SRC_DIR)/%.h $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ $(LIBRARIES)

# main compile
$(BIN): $(LIBS) $(SRC_DIR)/$(BIN).c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBRARIES)

# debug flags declaration
debug: CFLAGS=$(CFLAGS_DEBUG)
debug: clean $(BIN) gdb

gdb:
	sudo gdb $(BIN)

clean:
	rm -rf $(BIN) $(SRC_DIR)/*.o $(LIB_DIR)/*.o
