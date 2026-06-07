CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11
TARGET := oslabctl

.PHONY: all clean

all: $(TARGET)

$(TARGET): oslabctl.c
	$(CC) $(CFLAGS) -o $(TARGET) oslabctl.c

clean:
	rm -f $(TARGET)
