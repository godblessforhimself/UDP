#include "udpconst.h"
char mapping(int r)
{
	if (r >= 0 && r <= 9)
	{
		return '0' + r;
	}
	if (r >= 10 && r <= 35)
	{
		return 'a' + r - 10;
	}
	if (r >= 36 && r <= 61)
	{
		return 'A' + r - 36;
	}
	
	printf("mappig range error %d\n", r);
	return '0';
	
}
void genRand(char* msg, int len)
{
	/*0-9a-zA-Z[10+52=62]*/
	for (int i = 0; i < len; i ++)
	{
		msg[i] = mapping(rand() % 62);
	}
}
#define gen1rand(msg) genRand(msg, 1)
set<int> w_list;
void* sendRandom(void* args)
{
	
	int fd = *(int*)args;
	int len = *((int*)args + 1);
	sockaddr_in cliaddr = *(sockaddr_in*)((int*)args + 2);
	int new_ip = ntohl(cliaddr.sin_addr.s_addr);
	//printf("pthread args: len = %d, cliaddr address = %x\n", len, (sockaddr_in*)((int*)args + 1));
	
	print_addr(cliaddr);
	send_int(fd, MAGIC_HELLO, &cliaddr);
	//int fd = Socket(AF_INET, SOCK_DGRAM, 0);
	
	//set_reuse_addr(fd);
	
	/*len范围[1,intmax) 服务器要把len的随机字符发送给客户
		把len 拆分成times+1块，前times块大小为PACKET_LIMIT,最后一块大小为rest
		用滑动窗口控制丢包
		窗口大小window_size
		初始时上下界=0
		while
		{如果窗口大小<window_size，发包，上界++}
			{收到接收消息，计时器=0，对应块标记1，尝试移动窗口下界}
				{计时器满，没有消息，显示并发送未标记块}
					{
						上界到达times+1，下界也到达，结束
					}
	*/
	int window_size = 10;
	char* window = new char[PACKET_LIMIT * window_size];
	bool flags[window_size];
	memset(flags, 0, sizeof(bool) * window_size);
	int upper_bound = 0, lower_bound = 0;
	int times = len / PACKET_LIMIT;
	int rest = len - times * PACKET_LIMIT;
	char* msg = new char[PACKET_LIMIT + 4];
	char* pos = NULL;
	int actual = 0;
	#define findpos(window, upper) (window + (upper % window_size) * PACKET_LIMIT)
	#define setflag(upper_bound, x) flags[upper_bound % window_size] = x
	#define sendPackage(index) \
	pos = findpos(window, index);\
	int len = (index == times) ? rest : PACKET_LIMIT;\
	genRand(pos, len);\
	memcpy(msg, pos, len);\
	*(int*)(msg + PACKET_LIMIT) = index;\
	Send(fd, msg, PACKET_LIMIT + 4, &cliaddr);\
	setflag(index, 0);
	while (1)
	{
		if (upper_bound - lower_bound < window_size && upper_bound != times + 1)
		{
			//窗口未满，发包
			printf("send a package\n");
			sendPackage(upper_bound);
			upper_bound ++;
			continue;
		}
		if (upper_bound == lower_bound && lower_bound == times + 1)
		{
			printf("all sent ack\n");
			break;
		}
		printf("recv befor\n");
		int recvmsg = Recv(fd, msg, PACKET_LIMIT + 4, NULL);
		printf("recv %d bytes\n", recvmsg);
		if (recvmsg == TIMEOUT)
		{
			goto timeout;
		}
		for (int i = 0; i < recvmsg / 4; i ++)
		{
			int ack = *(int*)(msg + 4 * i);
			if (ack <= upper_bound && ack >= lower_bound)
			{
				setflag(ack, 1);
			}
			else
			{
				printf("ack error [%d < %d < %d]\n", lower_bound, ack, upper_bound);
			}
		}
		//尝试移动窗口下界
		while (flags[lower_bound % window_size])
		{
			flags[lower_bound % window_size] = 0;
			lower_bound ++;
			printf("lower_bound %d\n", lower_bound);
		}
		continue;
		timeout:
		//
		for (int i = lower_bound; i < upper_bound; i ++)
		{
			if (!flags[i % window_size])
			{
				printf("l%d i%d u%d\n", lower_bound, i, upper_bound);
				sendPackage(i);
			}
		}
	}

	
	printf("send all finish\n");
/* 	delete []msg;
	delete []pos;
	delete []window; */
	w_list.erase(new_ip);
}
int main(int argc, char* argv[])
{
	int sockfd;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	set_reuse_addr(sockfd);
	//set_buf_size(sockfd, MAXINT, MAXINT);
	//set_recv_time(sockfd, 3);
	Bind(sockfd, SERV_PORT);
	
	char msg[MAXLINE];
	
	struct sockaddr_in cliaddr;
	
	
	set<int>::iterator it;
	int n, new_ip, len, clifd;
	char args[sizeof(int) + sizeof(sockaddr_in)];
#define setarg(arg, fd, len, addr) *(int*)(arg) = (fd); *((int*)(arg) + 1) = (len); *(sockaddr_in*)(((int*)(arg)) + 2) = (addr);
	while (1)
	{
		n = Recv(sockfd, msg, MAXLINE, &cliaddr);
		len = *(int*)msg;
		new_ip = ntohl(cliaddr.sin_addr.s_addr);
		it = w_list.find(new_ip);
		if (it != w_list.end())
		{
			printf("waiting return\n");
		}
		else
		{
			clifd = Socket(AF_INET, SOCK_DGRAM, 0);
			set_recv_time(clifd, 1);
			if (Bind(clifd, 0) == BIND_ERROR)
			{
				printf("wait port\n");
				continue;
			}
			pthread_t tid;
			setarg(args, clifd, len, cliaddr);
			//printf("addr arg address %x\n", (sockaddr_in*)(((int*)(args)) + 1));
			w_list.insert(new_ip);
			printf("new ip %d\n", new_ip);
			pthread_create(&tid, NULL, sendRandom, args);
		}
	}
	
	
	
}