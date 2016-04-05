#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

size_t extract_argv(const char *s, char *argv[])
{
	size_t argc = 0;
	size_t n = strlen(s);
	for (size_t i = 0, start_arg = 0; i <= n; ++i)
		if (s[i] == ' ' || i == n)
		{
			if (start_arg < i)
			{
				size_t length = i - start_arg;
				argv[argc] = malloc(length + 1);
				strncpy(argv[argc], s + start_arg, length);
				argv[argc][length] = 0;
				++argc;
			}
			start_arg = i + 1;
		}
	argv[argc] = NULL;
	return argc;
}

int main()
{
	while (1)
	{
		printf("> ");
		char s[80];
		gets(s);
		if (s[0] == 'e' && !s[1])
			break;
		
		char *argv[10];
		size_t argc = extract_argv(s, argv);
		
		char in[80] = "";
		char out[80] = "";
		char err[80] = "";
		while (argc > 1)
		{
			char *arg = argv[argc - 1];
			if (arg[0] == '<')
				strcpy(in, arg + 1);
			else if (arg[0] == '>')
				strcpy(out, arg + 1);
			else if (arg[0] == '2' && arg[1] == '>')
				strcpy(err, arg + 2);
			else
				break;
			--argc;
			free(argv[argc]);
			argv[argc] = NULL;
		}
		
		pid_t pid = fork();
		check(pid, "Can't fork");
		if (pid == 0)
		{
			// потомок
			if (in[0])
			{
				int fd = open(in, O_RDONLY);
				check(fd, "Can't open in");
				dup2(fd, 0);
				close(fd);
			}
			
			if (out[0])
			{
				int fd = creat(out, 00664);
				check(fd, "Can't open out");
				dup2(fd, 1);
				close(fd);
			}
			
			if (err[0])
			{
				int fd = creat(err, 00664);
				check(fd, "Can't open err");
				dup2(fd, 2);
				close(fd);
			}
			
			int rc = execvp(argv[0], argv);
			check(rc, "Can't exec");
		}
		else
		{
			for (size_t i = 0; i < argc; ++i)
				free(argv[i]);
			wait(0);
		}
	}
	return 0;
}
