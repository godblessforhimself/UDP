#include "udpconst.h"

sockaddr_in* setAddr(const char* ip, int port)
{
	sockaddr_in* ret = new sockaddr_in();
	ret->sin_family = AF_INET;
	ret->sin_port = htons(port);
	inet_pton(AF_INET, ip, &(ret->sin_addr));
	return ret;
}
int wait_magic(int fd, sockaddr_in* &serv)
{
	sockaddr_in* new_comer = new sockaddr_in();
	char msg[4];
	memset(msg, 0, 4);
	while (Recv(fd, msg, 4, new_comer) != TIMEOUT)
	{
		if (*(int*)msg == MAGIC_HELLO)
		{
			serv = new_comer;
			return MAGIC_HELLO;
		}
	}
	return TIMEOUT;
}
void start_transfer(int fd, int len, sockaddr_in* serv)
{
	
	int window_size = 10;
	char* window = new char[PACKET_LIMIT * window_size];
	bool flags[window_size];
	memset(flags, 0, sizeof(bool) * window_size);
	int lower_bound = 0;
	int times = len / PACKET_LIMIT;
	int rest = len - times * PACKET_LIMIT;
	char* msg = new char[PACKET_LIMIT + 4];
	char* pos = NULL;
	sockaddr_in new_comer;
	int msg_len = 0, index = 0, success = 0;
	/*
		保证收到的msg是来自server的
		将msg拷贝到window里
		置flags[index]=1
		从lower_bound往上检查flags，开头连续的1的块输出并flag置0，bound++，若bound>times则结束
		发送index给服务器
	
	*/
	#define copy_to_window(index) \
	int len = (index == times) ? rest : PACKET_LIMIT;\
	pos = (window + (index % window_size) * PACKET_LIMIT); \
	memcpy(pos, msg, len);\
	flags[index % window_size] = 1;
	while (msg_len = Recv(fd, msg, PACKET_LIMIT + 4, &new_comer))
	{
		if (msg_len == TIMEOUT && success == 0)
		{
			printf("timeout\n");
			continue;
		}
		else if (msg_len == TIMEOUT && success)
		{
			printf("no more\n");
			break;
		}
		if (new_comer.sin_addr.s_addr != (serv->sin_addr).s_addr && new_comer.sin_port != serv->sin_port)
		{
			/*ignore msg come from not the server*/
			continue;
		}
		if (msg_len == PACKET_LIMIT + 4)
		{
			index = *(int*)(msg + PACKET_LIMIT);
			if (success)
			{
				send_int(fd, index, serv);
				//printf("waiting server to terminate\n");
				continue;
			}
			//printf("PACKET recv %d\n", index);
		}
		else
		{
			/*this package is not standard, ignore it*/
			//printf("NOT STANDARD msg_len = %d\n", msg_len);
			continue;
		}
		if (index < lower_bound || index > lower_bound + window_size)
		{
			/*index out of bound*/
			//printf("OUTOFBOUND [%d < %d < %d]\n", lower_bound, index, lower_bound + window_size);
			if (index < lower_bound && index >= 0)
			send_int(fd, index, serv);
			continue;
		}
		copy_to_window(index);
		while (flags[lower_bound % window_size])
		{
			if (lower_bound == times)
			{
				printf("%.*s", rest, (window + PACKET_LIMIT * (lower_bound % window_size)));
			}
			else
			{
				printf("%.*s", PACKET_LIMIT, (window + PACKET_LIMIT * (lower_bound % window_size)));
			}
			flags[lower_bound % window_size] = 0;
			lower_bound ++;
			//printf("low_bd %d\n", lower_bound);
		}
		
		send_int(fd, index, serv);
		
		if (lower_bound > times)
		{
			printf("\nall ack\n");
			success = 1;
			continue;
		}
	}
	
}
int main()
{
	int sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	set_buf_size(sockfd, 131071, 131071);
	set_recv_time(sockfd, 1);
	Bind(sockfd, CLI_PORT);
	sockaddr_in* serv = setAddr("127.0.0.1", SERV_PORT);
	
	while(1)
	{
		int n;
		cin >> n;
		serv = setAddr("127.0.0.1", SERV_PORT);
		send_int(sockfd, n, serv);
		while (wait_magic(sockfd, serv) == TIMEOUT)
		{
			send_int(sockfd, n, serv);
		}
		start_transfer(sockfd, n, serv);

	}
}