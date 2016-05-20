#include "base.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

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
		{
			if (errno == EPIPE)
				return -1;
			else
				perror("write"), exit(1);
		}
	}
	if (len == -1 && errno != EAGAIN)
		perror("read"), exit(1);
	
	if (fcntl(fromfd, F_SETFL, flags) == -1)
		perror("set fcntl 2"), exit(1);
	return sumlen;
}

int main(int argc, char **argv)
{
	// remove terminate after SIG_PIPE
	struct sigaction new_actn, old_actn;
	new_actn.sa_handler = SIG_IGN;
	sigemptyset(&new_actn.sa_mask);
	new_actn.sa_flags = 0;
	sigaction(SIGPIPE, &new_actn, &old_actn);
	
	// socket()
	int sockfd;
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
	
	// read & write
	printf("Hello to netchat client!\n");
	while (1)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(sockfd, &readfds);
		if (select(sockfd + 1, &readfds, NULL, NULL, NULL) == -1)
			perror("select"), exit(1);
		
		if (FD_ISSET(0, &readfds))
		{
			int sumlen = read_from_and_write_to(0, sockfd);
			if (sumlen == -1 && errno == EPIPE)
			{
				printf("Server disconnect (errno after read(...) == EPIPE\n");
				break;
			}
		}
		if (FD_ISSET(sockfd, &readfds))
		{
			int len = read_from_and_write_to(sockfd, 0);
			if (len == 0)
			{
				printf("Server disconnect (sumlen == 0)\n");
				break;
			}
		}
	}
	return 0;
}
