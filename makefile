CC := g++
CFLAGS := -std=c++17 -Wall -Wextra -pedantic -fPIC
LIB := st_reactor.so

SERVER_SRC := react_server.cpp
SERVER_OBJ := server.o

all:$(LIB) server 

$(LIB): $(SERVER_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $^

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@ -ldl

clean:
	rm -f $(LIB) server $(SERVER_OBJ)
.PHONY: all clean run