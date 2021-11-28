all: client server

client: client.c ConexaoRawSocket.o message.o common.o send_recieve.o
	gcc client.c ConexaoRawSocket.o message.o common.o send_recieve.o -o client

server: server.c ConexaoRawSocket.o message.o common.o send_recieve.o
	gcc server.c ConexaoRawSocket.o message.o common.o send_recieve.o -o server

ConexaoRawSocket.o: ConexaoRawSocket.c ConexaoRawSocket.h
	gcc -c ConexaoRawSocket.c -o ConexaoRawSocket.o

message.o: message.c message.h
	gcc -c message.c -o message.o

common.o: common.c common.h
	gcc -c common.c -o common.o

send_recieve.o: send_recieve.c send_recieve.h
	gcc -c send_recieve.c -o send_recieve.o

test: test.c
	gcc test.c -o test

clean: 
	rm -f *.o

purge: clean
	rm -f client server test
