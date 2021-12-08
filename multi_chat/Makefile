all: server client
	@echo "Compile chatting program..."

test: client_test.c server_test.c
	gcc -pthread -o server_test server_test.c
	gcc -pthread -o client_test client_test.c

server : server.c
	gcc -pthread -o server.o server.c

client : client.c
	gcc -pthread -o client.o client.c

clean:
	rm -f *.o