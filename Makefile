# Makefile for Lab 2 – A Simple Shell
# Author: Jeshurun Constantine
# Compiles myshell

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

# Target executable
TARGET = myshell

# Source files
SRC = myshell.c

# Default target: build shell
all: $(TARGET)

# Build executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up compiled files
clean:
	rm -f $(TARGET)
