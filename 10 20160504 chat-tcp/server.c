#include "base.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <assert.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

int client_sockfds[100];
int number_clients;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread_start(void *arg)
{
	int client_sockfd = (size_t) arg;
	char responce[1024];
	int len;
	while ((len = read(client_sockfd, responce, 1024)) > 0)
	{
		responce[len] = 0;
		printf("%d: %s", client_sockfd, responce);
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < number_clients; ++i)
			if (client_sockfds[i] != client_sockfd)
			{
				int rc = write(client_sockfds[i], responce, len);
				if (rc == -1)
				{
					if (errno == EPIPE)
						printf("\t %d disconnect (write(...) == EPIPE)\n", client_sockfd);
					else
						error(errno, errno, "write from %d to %d", client_sockfd, client_sockfds[i]);
				}
				else
					assert(rc == len);
			}
		pthread_mutex_unlock(&mutex);
	}
	if (len == -1)
		error(errno, errno, "read from %d", client_sockfd);
	else if (len == 0)
		printf("\t %d disconnect (read(...) == 0)\n", client_sockfd);
	
	// remove client from client_sockfds
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < number_clients; ++i)
		if (client_sockfds[i] == client_sockfd)
		{
			client_sockfds[i] = client_sockfds[number_clients - 1];
			client_sockfds[number_clients - 1] = 0;
			--number_clients;
			break;
		}
	pthread_mutex_unlock(&mutex);
	
	return NULL;
}

void handle_sig_int(int signal)
{
	printf("ctrl + c\n");
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < number_clients; ++i)
	{
		int rc = close(client_sockfds[i]);
		if (rc == -1)
			perror("close"), exit(1);
	}
	pthread_mutex_unlock(&mutex);
	exit(0);
}

int main(int argc, char **argv)
{
	// remove terminate after SIG_PIPE
	struct sigaction new_action, old_action;
	new_action.sa_handler = SIG_IGN;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGPIPE, &new_action, &old_action);
	
	// hanlde ctrl + c
	struct sigaction sig_int_action;
	sig_int_action.sa_handler = handle_sig_int;
	sigemptyset(&sig_int_action.sa_mask);
	sig_int_action.sa_flags = 0;
	sigaction(SIGINT, &sig_int_action, NULL);
	
	// socket()
	int server_sockfd;
	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		perror("socket"), exit(1);
	
	// for debug
	//int optval = 1;
	//if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1)
	//	perror("setsockopt SO_REUSEPORT"), exit(1);
	//optval = 1;
	//if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	//	perror("setsockopt SO_REUSEADDR"), exit(1);
	
	// bind
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	
	struct hostent *h;
	if ((h = gethostbyname("localhost")) == NULL)
		herror("gethostbyname"), exit(1);
	memcpy(&server_addr.sin_addr, h->h_addr, h->h_length);
	
	if (bind(server_sockfd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) == -1)
		perror("bind"), exit(1);
	
	// listen
	if (listen(server_sockfd, 0) == -1)
		perror("listen"), exit(1);
	
	// accept & pthread_create
	printf("Hello to netchat server!\n");
	while (1)
	{
		int client_sockfd;
		struct sockaddr_in client_addr;
		socklen_t client_addrlen = 0;
		if ((client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_addrlen)) == -1)
		{
			if (errno == EINTR)
				continue;
			perror("accept"), exit(1);
		}
		
		pthread_mutex_lock(&mutex);
		for (int i = 0; i <= number_clients; ++i)
			if (client_sockfds[i] == 0)
			{
				client_sockfds[i] = client_sockfd;
				break;
			}
		if (client_sockfds[number_clients] > 0)
			++number_clients;
		pthread_mutex_unlock(&mutex);
		printf("\t %d connect\n", client_sockfd);
		
		pthread_t id; //unsused
		pthread_create(&id, NULL, &thread_start, (void*) (size_t) client_sockfd);
	}
	return 0;
}
