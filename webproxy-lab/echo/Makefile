CC = gcc
CFLAGS = -g -Wall -I..

all: echo-client echo-server

csapp.o: ../csapp.c ../csapp.h
	$(CC) $(CFLAGS) -c ../csapp.c -o csapp.o

echo-client.o: echo-client.c ../csapp.h
	$(CC) $(CFLAGS) -c echo-client.c

echo-server.o: echo-server.c ../csapp.h
	$(CC) $(CFLAGS) -c echo-server.c

echo-client: echo-client.o csapp.o
	$(CC) $(CFLAGS) echo-client.o csapp.o -o echo-client

echo-server: echo-server.o csapp.o
	$(CC) $(CFLAGS) echo-server.o csapp.o -o echo-server

clean:
	rm -f *~ *.o echo-client echo-server
