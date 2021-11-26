all: client server

client: client.c ConexaoRawSocket.o message.o utils.o
	gcc client.c ConexaoRawSocket.o message.o utils.o -o client

server: server.c ConexaoRawSocket.o message.o utils.o
	gcc server.c ConexaoRawSocket.o message.o utils.o -o server

ConexaoRawSocket.o: ConexaoRawSocket.c ConexaoRawSocket.h
	gcc -c ConexaoRawSocket.c -o ConexaoRawSocket.o

message.o: message.c message.h
	gcc -c message.c -o message.o

utils.o: utils.c utils.h
	gcc -c utils.c -o utils.o

test: a.c
	gcc test.c -o test

clean: 
	rm -f *.o

purge: clean
	rm -f client server test
