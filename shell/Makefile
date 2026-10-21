CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Iinclude

SRC = src/main.c src/prompt.c src/parser.c src/intrinsics.c src/executor.c
OBJ = $(SRC:.c=.o)

TARGET = shell.out

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)
