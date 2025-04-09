CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = src/treasure_manager.c
OBJ = $(SRC:.c=.o)
OUT = treasure_manager

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ)

clean:
	rm -f $(OUT) src/*.o

.PHONY: all clean
