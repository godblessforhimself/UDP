all: server client

server: udpserver.cpp udpconst.h
	g++ udpserver.cpp -o ./server -std=c++11 -lpthread

client: udpclient.cpp udpconst.h
	g++ udpclient.cpp -o ./client -std=c++11 -lpthread



