# make file for hw2_chat_server.cpp

# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -std=c++11

# Target
TARGET = chat_server

# Source files
SRC = chat_server.cpp

# Compile
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean
clean:
	rm -f $(TARGET)

# Dependencies
chat_server.o: chat_server.cpp