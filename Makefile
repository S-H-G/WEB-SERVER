BIN = server
OBJECTS = server.o

all : $(BIN)

server : $(OBJECTS)
	gcc -o $@ $(OBJECTS)

server.o : server.c
	gcc -c -o $@ $<

clean :
	rm -rf *.o
