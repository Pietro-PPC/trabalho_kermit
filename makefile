all: client server

client: client.c ConexaoRawSocket.o
	gcc client.c ConexaoRawSocket.o -o client

server: server.c ConexaoRawSocket.o
	gcc server.c ConexaoRawSocket.o -o server

ConexaoRawSocket.o: ConexaoRawSocket.c ConexaoRawSocket.h
	gcc -c ConexaoRawSocket.c -o ConexaoRawSocket.o

clean: 
	rm -f *.o

purge: clean
	rm -f client server
