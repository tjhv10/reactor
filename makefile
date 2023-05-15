CC := g++
CFLAGS := -std=c++17 -Wall -Wextra -pedantic -fPIC

LIB := st_reactor.so

SERVER_SRC := st_reactor_server.cpp
SERVER_OBJ := server.o

all: run $(LIB) server 

$(LIB): $(SERVER_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $^

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -ldl

$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB) server $(SERVER_OBJ)

run: server $(LIB)
	LD_LIBRARY_PATH=./st_reactor

.PHONY: all clean run
