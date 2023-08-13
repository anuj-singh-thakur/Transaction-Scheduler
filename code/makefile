# Makefile for DistributedTransactionScheduler.cpp

# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -g

# Source files
SRCS = DistributedTransactionScheduler.cpp

# Output binary name
TARGET = DistributedTransactionScheduler.out

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

.PHONY: clean

clean:
	rm -f $(TARGET)
