#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

struct proc
{
	int pid;
	int ppid;
};

struct proc init;

void add(const char *s)
{
	printf("%s\n", full_path);
	char stat[256] = "/proc/";
	strcat(full_path, entry->d_name);
	FILE* stat = open
}

int main()
{
	DIR *dir = opendir("/proc");
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
		if (entry->d_name[0] != '.')
		{
			char full_path[256] = "/proc/";
			strcat(full_path, entry->d_name);
			add(full_path);
		}
	closedir(dir);
	return 0;
}
