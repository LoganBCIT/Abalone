# Makefile for Abalone project

# Compiler and flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# Target name
TARGET   = abalone

# Source and object files
SRC      = main.cpp Board.cpp
OBJS     = main.o Board.o

all: $(TARGET)

# Link step: produce the final executable from object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Compile each .cpp into .o
main.o: main.cpp Board.h
	$(CXX) $(CXXFLAGS) -c main.cpp

Board.o: Board.cpp Board.h
	$(CXX) $(CXXFLAGS) -c Board.cpp

# Optional: remove the executable and object files
clean:
	rm -f $(TARGET) $(OBJS)
