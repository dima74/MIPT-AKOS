#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

void in(const char *s)
{
	return;
	printf("%s...", s);
	scanf("%*c");
}

int read_from_and_write_to(int fromfd, int tofd)
{
	//printf("from %d to %d\n", fromfd, tofd);
	
	int flags;
	if ((flags = fcntl(fromfd, F_GETFL, 0)) == -1)
		perror("get fcntl"), exit(1);
	if (fcntl(fromfd, F_SETFL, flags | O_NONBLOCK) == -1)
		perror("set fcntl 1"), exit(1);
	
	int sumlen = 0;
	char buffer[4];
	int len;
	while ((len = read(fromfd, buffer, sizeof(buffer))) > 0)
	{
		sumlen += len;
		int rc = write(tofd, buffer, len);
		if (rc == -1)
			perror("write"), exit(1);
	}
	if (len == -1 && errno != EAGAIN)
		perror("read"), exit(1);
	
	if (fcntl(fromfd, F_SETFL, flags) == -1)
		perror("set fcntl 2"), exit(1);
	return len;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("Usage: %s host port\n", argv[0]);
		exit(1);
	}
	
	char *host = argv[1];
	int port = strtol(argv[2], NULL, 10);
	if (errno == ERANGE)
		perror("Invalid port"), exit(1);
	
	in("socket");
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		perror("socket"), exit(1);
	
	/*
	// don't leave the socket in a TIME_WAIT state if we close the connection
	struct linger fix_ling;
	fix_ling.l_onoff = 1;
	fix_ling.l_linger = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &fix_ling, sizeof(fix_ling)) == -1)
		perror("setsockopt"), exit(1);
	//*/
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	struct hostent *h;
	if ((h = gethostbyname(host)) == NULL)
		herror("gethostbyname"), exit(1);
	memcpy(&addr.sin_addr, h->h_addr, h->h_length);
	
	in("connect");
	if (connect(sockfd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1)
		perror("connect"), exit(1);
	
	while (1)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(sockfd, &readfds);
		//printf("select...\n");
		if (select(sockfd + 1, &readfds, NULL, NULL, NULL) == -1)
			perror("select"), exit(1);
		
		if (FD_ISSET(sockfd, &readfds))
		{
			int len = read_from_and_write_to(sockfd, 0);
			if (len == 0)
				break;
		}
		if (FD_ISSET(0, &readfds))
			read_from_and_write_to(0, sockfd);
		
		/*
		int error = 0;
		socklen_t len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, , &error, &len) == -1)
			perror("getsockopt"), exit(1);
		if (error == -1)
			perror("SO_ERROR"), exit(1);//*/
	}
	
	return 0;
}
