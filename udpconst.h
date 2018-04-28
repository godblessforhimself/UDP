#include <unistd.h>
#include <errno.h>
#include <string.h> //strerror
#include <stdio.h>  //printf
#include <sys/types.h>  //socket recvfrom sendto
#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in
#include <iostream>
#include <sys/epoll.h>  //epoll
#include <pthread.h> //pthread create
#include <set>
using namespace std;
#define MAXLINE 8
#define PACKET_LIMIT (65507 - 4)
#define SERV_PORT 7023

#define CLI_PORT 2307
#define TIMEOUT -1
#define BIND_ERROR -1
#define MAGIC_HELLO -42941
void set_send_time(int fd, int time)
{
	struct timeval timeout = {time, 0};
	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)))
	{
		cout << "set receive time error:  " << errno << endl;
	}
}
void set_recv_time(int fd, int time)
{
	struct timeval timeout = {time, 0};
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)))
	{
		cout << "set receive time error:  " << errno << endl;
	}
}
void print_addr(sockaddr_in addr)
{
	printf("ip = %s, port = %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

void set_reuse_addr(int fd)
{
	int opt_val = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)))
	{
		cout << "set reuseaddr error:  " << errno << endl;
	}
}
void set_reuse_port(int fd)
{
	int opt_val = 1;
 	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt_val, sizeof(opt_val)))
	{
		cout << "set reuseport error:  " << errno << endl;
	} 
}
void set_buf_size(int fd, int snd, int rcv)
{
	
	if (!setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcv, sizeof(rcv)))
	{
		//printf("rcvbuf len = %d\n", rcv);
	}
	
	if (!setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &snd, sizeof(snd)))
	{
		//printf("sendbuf len = %d\n", snd);
	}
}
int Socket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}
int Bind(int fd, int port)
{
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = 0;
	if (port != 0)
	servaddr.sin_port = htons(port);
	if (bind(fd, (sockaddr*)&servaddr, sizeof(sockaddr_in)))
	{
		printf("Bind Error %s\n", strerror(errno));
		return BIND_ERROR;
	}
	return 0;
}

int Recv(int fd, char* msg, int len, struct sockaddr_in* addr)
{

	int addrlen = sizeof(struct sockaddr_in), ret;
	/*recvfrom 合法返回字节数[0,len] */
	if (addr)
	ret = recvfrom(fd, msg, len, 0, (sockaddr*)addr, (socklen_t*)&addrlen);
	else
	ret = recvfrom(fd, msg, len, 0, NULL, NULL);

	if (ret < 0 || ret > len)
	{
		printf("recv return value = %d, %s\n", ret, strerror(errno));
		if (ret == -1)
		{
			return TIMEOUT;
		}
	}
	//print_addr(*addr);
	return ret;
}

int Send(int fd, char* msg, int len, struct sockaddr_in* addr)
{
	#if 0
	printf("send\n");
	#endif
	/*sendto 合法返回值[0,len],len才是想要的*/
	socklen_t tolen = sizeof(struct sockaddr_in);
	int ret = sendto(fd, (void*)msg, len, 0, (sockaddr*)addr, tolen);
	if (ret < 0 || ret > len)
	{
		printf("send return value = %d, errno = %s\n", ret, strerror(errno));
		if (ret == -1)
		{
			return TIMEOUT;
		}
	}
	else if (ret != len)
	{
		printf("send actual %d bytes\n", ret);
	}
	#if 0
	printf("%d\n", ret);
	#endif
	return ret;
}

void send_int(int fd, int a, sockaddr_in* addr)
{
	if (Send(fd, (char*)&a, 4, addr) != 4)
	{
		printf("sent_int fail \n");
	}
}







