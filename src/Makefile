CC=gcc
CFLAGS_DEBUG=-g -Wall -O0
CFLAGS_BUILD=-O2

SRC_DIR=src
LIB_DIR=src/lib

LIBS=$(LIB_DIR)/logger.o $(SRC_DIR)/mlib.o $(SRC_DIR)/utils.o

LIBRARIES=-lcurl -pthread -lsystemd

BIN=dyn-dns

# default standard build
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

prod: CFLAGS=$(CFLAGS_BUILD)
prod: clean $(BIN)

# debug flags declaration
debug: CFLAGS=$(CFLAGS_DEBUG)
debug: $(BIN)

gdb:
	sudo gdb $(BIN)

clean:
	rm -rf $(BIN) $(SRC_DIR)/*.o $(LIB_DIR)/*.o
