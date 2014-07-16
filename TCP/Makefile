COMPILER=gcc
CFLAGS=-Wall
all: echotcp
echotcp: server client
server:
$(COMPILER) tcpserver.c -o tcpserver
client:
$(COMPILER) tcpclient.c -o tcpclient