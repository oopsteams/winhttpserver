# Makefile

CC = g++
CFLAGS = -Wall -c -g -DWIN32 -lwsock32 -lws2_32 

all: server
# $(CC) -Wl,--subsystem,windows -o server server.o Socket.o error.o SocketStream.o HttpSocket.o -lwsock32  -lws2_32 -lgdi32
server: server.o HttpSocket.o Socket.o error.o SocketStream.o 
	$(CC) -o server server.o Socket.o error.o SocketStream.o HttpSocket.o -lwsock32  -lws2_32 -lgdi32  
# $(CC) -mwindows -o server server.o Socket.o error.o SocketStream.o HttpSocket.o -lwsock32  -lws2_32 -lgdi32  
server.o: server.cpp HttpSocket.h
	$(CC) $(CFLAGS) server.cpp 
Socket.o: Socket.cpp Socket.h package.h SocketStream.h error.h
	$(CC) $(CFLAGS) Socket.cpp
HttpSocket.o: HttpSocket.h HttpSocket.cpp 
	$(CC) $(CFLAGS) HttpSocket.cpp
error.o: error.cpp error.h
	$(CC) $(CFLAGS) error.cpp
SocketStream.o: SocketStream.cpp SocketStream.h
	$(CC) $(CFLAGS) SocketStream.cpp
clean:
	rm *.o 
