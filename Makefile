# Makefile для 8088 Emulator IBM PC XT
CC = g++
CFLAGS = -Wall -g -std=c++17
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
SOURCES = keyboard.cpp loader.cpp fdd.cpp joystick.cpp video.cpp opcode_functions.cpp Tester.cpp Tester2.cpp Tester3.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = 8088_emulator

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
    $(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
