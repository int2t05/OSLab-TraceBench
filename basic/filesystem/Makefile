CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -Iinclude
TARGET := filesystem
SRCS := src/main.c src/parser.c src/filesystem.c src/output.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) include/filesystem.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) src/*.o
