CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -Iinclude
LDFLAGS ?= -pthread
TARGET := sync
SRCS := src/main.c src/parser.c src/producer_consumer.c src/readers_writers.c src/dining_philosophers.c src/output.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) include/sync.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET) src/*.o
