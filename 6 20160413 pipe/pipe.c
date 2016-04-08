#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

void check(int rc, const char *message)
{
	if (rc == -1)
	{
		printf("%s: %s\n", message, strerror(errno));
		exit(1);
	}
}

int extract_argv(const char *s, char *argv[])
{
	int argc = 0;
	int n = strlen(s);
	for (int i = 0, start_arg = 0; i <= n; ++i)
		if (i == n || s[i] == ' ')
		{
			if (start_arg < i)
			{
				int length = i - start_arg;
				argv[argc] = malloc(length + 1);
				strncpy(argv[argc], s + start_arg, length);
				argv[argc][length] = 0;
				++argc;
			}
			start_arg = i + 1;
		}
	return argc;
}

/*
cat <1.txt | sort | sort | grep 1 | wc >2.txt
*/
int extract_redirect(char *argv[], int argc, char in[], char out[], char err[])
{
	for (int i = 0; i < argc; ++i)
	{
		char *arg = argv[i];
		if (arg[0] == '<')
			strcpy(in, arg + 1);
		else if (arg[0] == '>')
			strcpy(out, arg + 1);
		else if (arg[0] == '2' && arg[1] == '>')
			strcpy(err, arg + 2);
		else
			continue;
		free(argv[i]);
		for (int j = i; j < argc; ++j)
			argv[j] = argv[j + 1];
		--argc;
		--i;
	}
	return argc;
}

int main()
{
	while (1)
	{
		printf("> ");
		char s[80];
		gets(s, 80);
		if (s[0] == 'e' && !s[1])
			break;
		
		char *argv[20];
		int argc = extract_argv(s, argv);
		
		char in[80] = "", out[80] = "", err[80] = "";
		argc = extract_redirect(argv, argc, in, out, err);
		
		pid_t last_pid;
		int ichild = 0;
		int next_in = -1;
		int iargv0 = 0;
		for (int i = 0; i <= argc; ++i)
			if (i == argc || strcmp(argv[i], "|") == 0)
			{
				int pipefd[2] = {-1, -1};
				if (i < argc)
				{
					int rc = pipe(pipefd);
					check(rc, "Can't pipe");
				}
				// swap(pipefd[0], next_in);
				int temp = next_in;
				next_in = pipefd[0];
				pipefd[0] = temp;
				
				pid_t pid = fork();
				last_pid = pid;
				check(pid, "Can't fork");
				if (pid == 0)
				{
					// потомок
					if (next_in != -1)
					{
						close(next_in);
						next_in = -1;
					}
					
					// in
					{
						int fd = pipefd[0];
						if (ichild == 0 && in[0])
						{
							fd = open(in, O_RDONLY);
							check(fd, "Can't open in");
						}
						if (fd != -1)
						{
							dup2(fd, 0);
							close(fd);
						}
					}
					
					// out
					{
						int fd = pipefd[1];
						if (i == argc && out[0])
						{
							fd = creat(out, 00664);
							check(fd, "Can't open out");
						}
						if (fd != -1)
						{
							dup2(fd, 1);
							close(fd);
						}
					}
					
					if (err[0])
					{
						int fd = creat(err, 00664);
						check(fd, "Can't open err");
						dup2(fd, 2);
						close(fd);
					}
					
					argv[i] = 0;
					int rc = execvp(argv[iargv0], argv + iargv0);
					fprintf(stderr, "rc %d\n", rc);
					check(rc, "Can't exec");
				}
				else
				{
					++ichild;
					if (pipefd[0] != -1)
						close(pipefd[0]);
					if (pipefd[1] != -1)
						close(pipefd[1]);
					iargv0 = i + 1;
				}
			}
		for (int i = 0; i < argc; ++i)
			free(argv[i]);
		waitpid(last_pid, 0, 0);
	}
	return 0;
}
