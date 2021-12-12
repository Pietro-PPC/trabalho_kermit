all: client server

client: client.c ConexaoRawSocket.o message.o common.o send_recieve.o files_and_dirs.o 
	gcc client.c ConexaoRawSocket.o message.o common.o send_recieve.o files_and_dirs.o -o client -pthread

server: server.c ConexaoRawSocket.o message.o common.o send_recieve.o files_and_dirs.o
	gcc server.c ConexaoRawSocket.o message.o common.o send_recieve.o files_and_dirs.o -o server -pthread

ConexaoRawSocket.o: ConexaoRawSocket.c ConexaoRawSocket.h
	gcc -c ConexaoRawSocket.c -o ConexaoRawSocket.o

message.o: message.c message.h
	gcc -c message.c -o message.o

common.o: common.c common.h
	gcc -c common.c -o common.o

send_recieve.o: send_recieve.c send_recieve.h
	gcc -c send_recieve.c -o send_recieve.o 

files_and_dirs.o: files_and_dirs.c files_and_dirs.h
	gcc -c files_and_dirs.c -o files_and_dirs.o

test: test.c
	gcc test.c -o test -pthread

clean: 
	rm -f *.o

purge: clean
	rm -f client server test
