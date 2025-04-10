CC = gcc
CFLAGS = -Wall -Wextra -g
SRC_DIR = src
TARGET = treasure_manager

SRC = $(SRC_DIR)/treasure_manager.c
OBJ = $(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
