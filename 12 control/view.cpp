#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>

#include "base.h"

int sockfd;

void reopen()
{
	// socket()
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		perror("socket"), exit(1);
	
	// connect()
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	
	struct hostent *h;
	if ((h = gethostbyname("localhost")) == NULL)
		herror("gethostbyname"), exit(1);
	memcpy(&addr.sin_addr, h->h_addr, h->h_length);
	
	if (connect(sockfd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1)
		perror("connect"), exit(1);
}

void printinfo()
{
	reopen();
	char request[10] = "getinfo";
	check(write(sockfd, request, strlen(request)));
	
	deque<uint8_t> data;
	char responce[1024];
	int len;
	//dbg(0);
	while ((len = read(sockfd, responce, sizeof(responce))) > 0)
	{
		//dbg(0, len);
		for (int i = 0; i < len; ++i)
			data.push_back(responce[i]);
	}
	check(len);
	//dbg(1, len);
	
	int size = data.front(); data.pop_front();
	//dbg(size);
	vector<Program> ps(size);
	for (Program &p : ps)
		p.read(data);
	
	printf("Number programs: %d\n", size);
	for (Program &p : ps)
		cout << p << endl;
}

void reload()
{
	reopen();
	char request[10] = "reload";
	dbg(0);
	check(write(sockfd, request, strlen(request)));
	dbg(1);
}

int main()
{
	
	
	while (1)
	{
		printf("[1] -- info\n");
		printf("[2] -- reload\n");
		int type = 0;
		if (type == 0)
			scanf("%d", &type);
		switch (type)
		{
			case 1:
				printinfo();
				break;
			case 2:
				reload();
				break;
			default:
				printf("It's not a command\n");
				break;
		}
	}
	return 0;
}
