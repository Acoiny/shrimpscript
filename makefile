CC = g++

OPT = -O2

BINARY=shrimp

INCDIRS = ./header ./header/commandLineOutput ./header/nativeFunctions ./header/virtualMachine
CODEDIRS = ./source ./source/commandLineOutput ./source/nativeFunctions ./source/virtualMachine

CXXFLAGS = -std=c++20 -Wall $(OPT) $(foreach DIR, $(INCDIRS), -I$(DIR))

CXXFILES=$(foreach DIR, $(CODEDIRS),$(wildcard $(DIR)/*.cpp))

OBJECTS=$(patsubst %.cpp, %.o, $(CXXFILES))

all=$(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -rf $(BINARY) $(OBJECTS)