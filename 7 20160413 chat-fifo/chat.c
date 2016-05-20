#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <semaphore.h>

#define NAME_PIPE "/tmp/chat_pipe"
#define NAME_SEM_FIFO "/chat_sem_fifo"
#define NAME_SEM_READ "/chat_sem_read"

int main(int argc, char **argv)
{
	printf("Hello to chat 1.0!\n");
	char s[80];
	
	struct pollfd pfds[2];
	pfds[0].fd = 0;
	pfds[0].events = POLLIN;
	pfds[1].events = POLLIN;
	
	if (mkfifo(NAME_PIPE, 0777) && errno != EEXIST)
		perror("mkfifo"), exit(1);
	
	if ((pfds[1].fd = open(NAME_PIPE, O_WRONLY | O_NONBLOCK)) == -1)
	{
		if (errno != ENXIO)
			perror("write only open"), exit(1);
		else
		{
			if (sem_unlink(NAME_SEM_FIFO) == -1 && errno != ENOENT)
				error(1, errno, "sem_unlink");
			if (sem_unlink(NAME_SEM_READ) == -1 && errno != ENOENT)
				error(1, errno, "sem_unlink");
		}
	}
	else
		close(pfds[1].fd);
	
	if ((pfds[1].fd = open(NAME_PIPE, O_RDONLY | O_NONBLOCK)) == -1)
		perror("open 1"), exit(1);
	
	sem_t *sem_fifo_now;
	sem_t *sem_read_done;
	if ((sem_fifo_now = sem_open(NAME_SEM_FIFO, O_CREAT, 0777, 1)) == SEM_FAILED
	|| (sem_read_done = sem_open(NAME_SEM_READ, O_CREAT, 0777, 0)) == SEM_FAILED)
		perror("sem_open"), exit(1);
	
	printf("wait\n");
	sem_wait(sem_fifo_now);
	printf("post\n");
	sem_post(sem_fifo_now);
	
	while (1)
	{
		if (poll(pfds, 2, -1) == -1)
			perror("poll"), exit(1);
		
		//printf("wait sem_write_now\n");
		sem_wait(sem_fifo_now);
		//printf("  done wait sem_write_now\n");
		//printf("another\t %d\n", pfds[1].revents & POLLIN);
		//printf("stdin\t %d\n", pfds[0].revents & POLLIN);
		bool write_to_fifo = 0;
		if (pfds[1].revents & POLLIN)
		{
			// another process in
			int len;
			if ((len = read(pfds[1].fd, s, 80)) == -1)
			{
				//perror("read another");
				printf("Missing broadcast\n");
				sem_post(sem_fifo_now);
				continue;
				//return 1;
			}
			sem_post(sem_read_done);
			write(pfds[0].fd, s, len);
		}
		
		if (pfds[0].revents & POLLIN)
		{
			// stdin
			int len;
			if ((len = read(pfds[0].fd, s, 80)) == -1)
				perror("read stdin"), exit(1);
			
			close(pfds[1].fd);
			if ((pfds[1].fd = open(NAME_PIPE, O_WRONLY)) == -1)
				perror("write"), exit(1);
			write_to_fifo = 1;
			write(pfds[1].fd, s, len);
			if ((pfds[1].fd = open(NAME_PIPE, O_RDONLY | O_NONBLOCK)) == -1)
				perror("open 2"), exit(1);
		}
		sem_post(sem_fifo_now);
		//printf("wait sem_read_done\n");
		if (write_to_fifo)
			sem_wait(sem_read_done);
		//printf("  done wait sem_read_done\n");
	}
	
	close(pfds[1].fd);
	sem_close(sem_fifo_now);
	sem_close(sem_read_done);
	return 0;
}
