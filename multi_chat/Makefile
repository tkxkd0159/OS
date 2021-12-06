all: server client
	@echo "Compile chatting program..."

server : server.c
	gcc -pthread -o server.o server.c

client : client.c
	gcc -pthread -o client.o client.c

clean:
	rm -f *.o