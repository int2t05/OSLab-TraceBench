CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -Iinclude
TARGET := memory
SRCS := src/main.c src/parser.c src/partition.c src/paging.c src/output.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) include/memory.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) src/*.o
