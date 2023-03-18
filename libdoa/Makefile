CC = g++
BIN = libdoaexample

SOURCES = example.cpp doa.cpp doa.h
INCLUDES = -I./

$(BIN): $(SOURCES)
	$(CC) -g $(SOURCES) $(INCLUDES) -o $(BIN)

clean:
	rm -rf $(BIN)