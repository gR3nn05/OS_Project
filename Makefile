CC = gcc
CFLAGS = -Wall -Wextra -g
SRC_DIR = src
BIN_DIR = .

all: treasure_manager treasure_hub

# Treasure Manager
treasure_manager: $(SRC_DIR)/treasure_manager.o
	$(CC) $(CFLAGS) -o $(BIN_DIR)/treasure_manager $(SRC_DIR)/treasure_manager.o

$(SRC_DIR)/treasure_manager.o: $(SRC_DIR)/treasure_manager.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/treasure_manager.c -o $(SRC_DIR)/treasure_manager.o

# Treasure Hub
treasure_hub: $(SRC_DIR)/treasure_hub.o $(SRC_DIR)/monitor.o
	$(CC) $(CFLAGS) -o $(BIN_DIR)/treasure_hub $(SRC_DIR)/treasure_hub.o $(SRC_DIR)/monitor.o

$(SRC_DIR)/treasure_hub.o: $(SRC_DIR)/treasure_hub.c $(SRC_DIR)/monitor.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/treasure_hub.c -o $(SRC_DIR)/treasure_hub.o

$(SRC_DIR)/monitor.o: $(SRC_DIR)/monitor.c $(SRC_DIR)/monitor.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/monitor.c -o $(SRC_DIR)/monitor.o


# Create symbolic links
symlinks: all
	ln -sf $(BIN_DIR)/treasure_manager treasure_manager_link
	ln -sf $(BIN_DIR)/treasure_hub treasure_hub_link
	ln -sf $(BIN_DIR)/monitor monitor_link

# Clean
clean:
	rm -f $(SRC_DIR)/*.o $(BIN_DIR)/treasure_manager $(BIN_DIR)/treasure_hub $(BIN_DIR)/monitor treasure_manager_link treasure_hub_link monitor_link