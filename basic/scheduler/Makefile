CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -Iinclude
TARGET := scheduler
SRCS := src/main.c src/parser.c src/scheduler.c src/output.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) include/scheduler.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) src/*.o
